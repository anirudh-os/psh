#include "psh.h"
char path_history[MAX_HISTORY][PATH_MAX];
int history_index = 0;
char cwd[1024];
char *builtin_str[] = {"exit", "cd", "echo", "pwd", "fc"};
int (*builtin_func[])(char **) = {&PSH_EXIT, &PSH_CD, &PSH_ECHO, &PSH_PWD,
                                  &PSH_FC};
int size_builtin_str = sizeof(builtin_str) / sizeof(builtin_str[0]);

int PSH_EXIT(char **token_arr) {
  if (!token_arr[1]) {
    printf("bye bye PSH :D\n"); // handling empty args and freeing token array
                                // before leaving
    free(token_arr);
    delete_file(SESSION_HISTORY_FILE);
    return 0;
    // exit(0);
  }
  printf("bye bye PSH :D\n");
  int exit_code = atoi(token_arr[1]);
  free(token_arr);
  delete_file(SESSION_HISTORY_FILE);
  return exit_code;
  // exit(exit_code);char
}

int PSH_CD(char **token_arr) {
  char current_dir[PATH_MAX];
  char *localdir = malloc(PATH_MAX);
  char *home = NULL;                   // for ~ and empty cases
  static char PREV_DIR[PATH_MAX] = ""; // for - cases

  if (localdir == NULL) {
    perror("PSH: malloc() error");
    return 1;
  }

  if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
    perror("PSH: getcwd() error");
    free(localdir);
    return 1;
  }

  if (token_arr[1] == NULL || strcmp(token_arr[1], "~") == 0) {
    home = getenv("HOME");
    if (home == NULL) {
      fprintf(stderr, "PSH: HOME environment variable not set\n");
      free(localdir);
      return 1;
    }
    strncpy(localdir, home, PATH_MAX - 1);
    localdir[PATH_MAX - 1] = '\0';
  } else if (strcmp(token_arr[1], "-") == 0) {
    if (PREV_DIR[0] == '\0') {
      fprintf(stderr, "PSH: No previous directory\n");
      free(localdir);
      return 1;
    }
    strncpy(localdir, PREV_DIR, PATH_MAX - 1);
    localdir[PATH_MAX - 1] = '\0';
  } else if (strcmp(token_arr[1], "..") == 0) {
    if (history_index > 0) {
      history_index--;
      strncpy(localdir, path_history[history_index], PATH_MAX - 1);
      localdir[PATH_MAX - 1] = '\0';
    } else {
      snprintf(localdir, PATH_MAX, "%s/..", current_dir);
    }
  } else {
    if (token_arr[1][0] != '/') {
      snprintf(localdir, PATH_MAX, "%s/%s", current_dir, token_arr[1]);
    } else {
      strncpy(localdir, token_arr[1], PATH_MAX - 1);
      localdir[PATH_MAX - 1] = '\0';
    }
  }

  char resolved_path[PATH_MAX];
  if (resolve_and_manage_symlink(localdir, resolved_path) == -1) {
    free(localdir);
    return 1;
  }

  // Save the logical path in the history
  if (token_arr[1] != NULL && strcmp(token_arr[1], "..") != 0 &&
      strcmp(token_arr[1], "-") != 0) {
    strncpy(path_history[history_index], resolved_path, PATH_MAX - 1);
    path_history[history_index][PATH_MAX - 1] = '\0';
    history_index++;
    // printf("%s\n",*path_history);
  }

  if (strcmp(localdir, resolved_path) != 0) {
    printf("PSH: followed symlink: %s -> %s\n", localdir, resolved_path);
  }

  strncpy(PREV_DIR, current_dir, PATH_MAX - 1);
  PREV_DIR[PATH_MAX - 1] = '\0';

  free(localdir);
  return 1;
}

int PSH_ECHO(char **token_arr) { //
  // Sid
  //   int i = 1;
  //   printf("%s",token_arr[0]); // debugging
  //   // while (token_arr[i] != "\n") {
  //   // printf("%s",token_arr[i]);
  //   // }
  return 1;
}

int PSH_PWD(char **token_arr) {

  // printf("%s\n", token_arr[1]); //debugging
  // printf("Printing current working directory\n"); //debugging

  char rpath[1024];
  // printf("%d\n", PATH_MAX);

  if (token_arr[1] == NULL ||
      strcmp(token_arr[1], "-L") == 0) // Default pwd and pwd -L
  {
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
    } else {
      perror("PSH: getcwd() error");
    }
  } else if ((strcmp(token_arr[1], "-P")) == 0) {
    if (realpath(cwd, rpath) != NULL) // pwd -P
    {
      printf("%s\n", rpath);
    } else {
      perror("PSH: realpath() error");
    }
  } else {
    fprintf(stderr, "Unknown option: %s\n", token_arr[1]);
  }
  return 1;
}

int PSH_FC(char **token_arr) {
  int n = -1;

  if (token_arr[1] == NULL) {
    n = 0;
  } else if ((strcmp(token_arr[1], "-l") == 0) &&
             (token_arr[2] == NULL ||
              (strcmp(token_arr[2], "-n") != 0 &&
               strcmp(token_arr[2], "-r") != 0))) { // fc -l 5 10
    n = 1;

  } else if (((strcmp(token_arr[1], "-ln")) == 0) ||  
             ((strcmp(token_arr[1], "-nl")) == 0) ||
             (((strcmp(token_arr[1], "-l")) == 0) &&
              ((strcmp(token_arr[2], "-n")) == 0)) ||
             (((strcmp(token_arr[1], "-n")) == 0) &&
              ((strcmp(token_arr[2], "-l")) ==
               0))) { // fc -ln, -nl, -l -n, -n -l
    n = 2;
  } else if (((strcmp(token_arr[1], "-lr")) == 0) ||
             ((strcmp(token_arr[1], "-rl")) == 0) ||
             (((strcmp(token_arr[1], "-l")) == 0) &&
              ((strcmp(token_arr[2], "-r")) == 0)) ||
             (((strcmp(token_arr[1], "-r")) == 0) &&
              ((strcmp(token_arr[2], "-l")) ==
               0))) { // fc -lr, -rl, -l -r, -r -l
    n = 3;
  } else if ((strcmp(token_arr[1], "-e")) == 0) {
    n = 4;
  } else if ((strcmp(token_arr[1], "-s")) == 0) {
    n = 5;
  } else if ((strcmp(token_arr[1], "-d")) == 0) {
    n = 6;
  } else if ((strcmp(token_arr[1], "-c")) == 0) {
    n = 7;
  } else if ((strcmp(token_arr[1], "-p")) == 0) {
    n = 8;
  } else {
    printf("psh: fc: missing history argument");
    return 1;
  }
  switch (n) {

  case 0: {
    printf("psh: fc: missing history argument");
    break;
  }
    // fc -l 20 50
  case 1: { // SEGMENTATION FAULT FOR just fc -l. Others work
    if (token_arr[2] == NULL) {
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines(MEMORY_HISTORY_FILE, 1, total_lines);
    } else if (token_arr[3] == NULL) { // fc -l 5 10
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines(MEMORY_HISTORY_FILE, atoi(token_arr[2]), total_lines);
    } else {
      read_lines(MEMORY_HISTORY_FILE, atoi(token_arr[2]), atoi(token_arr[3]));
    }
    break;
  }

  case 2: { // fc -l -n 20 50 (works)      fc -ln 20 50 (doesn't work) NEEDS TO
            // BE FIXED
    if (token_arr[2] == NULL) {
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines_wo_no(MEMORY_HISTORY_FILE, 1, total_lines);
    }

    else if (token_arr[2] != NULL && token_arr[3] == NULL) {
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[2]), total_lines);

    } else if (token_arr[4] == NULL) {
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[3]), total_lines);

    } else if (token_arr[3] != NULL && token_arr[4] != NULL) {
      read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[3]),
                       atoi(token_arr[4]));

    } else if (token_arr[2] != NULL && token_arr[3] != NULL && token_arr[4]==NULL) {
      read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[3]),
                       atoi(token_arr[4]));
    }

    break;
  }

  case 3: { // fc -l -r 20 50 (works)      fc -lr 20 50 (doesn't work)
    if (token_arr[2] == NULL) {
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines_reverse(MEMORY_HISTORY_FILE, 1, total_lines);
    }

    else if (token_arr[2] != NULL && token_arr[3] == NULL) {
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines_reverse(MEMORY_HISTORY_FILE, atoi(token_arr[2]), total_lines);
    } else if (token_arr[4] == NULL) {
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      read_lines_reverse(MEMORY_HISTORY_FILE, atoi(token_arr[3]), total_lines);
    } else {
      read_lines_reverse(MEMORY_HISTORY_FILE, atoi(token_arr[3]),
                         atoi(token_arr[4]));
    }

    break;
  }

  case 4: { // fc -e
    if (token_arr[2] == NULL) {
      printf("psh: fc: -e requires an editor name\n");
      return 1;
    }

    const char *editor = token_arr[2];
    const char *temp_file = "/tmp/psh_fc_edit.tmp";
    char command[1024];
    int start, end;

    // Determining range of commands to edit
    if (token_arr[3] == NULL) {
      // Edit last command by default
      int total_lines = count_lines(MEMORY_HISTORY_FILE);
      start = end = total_lines;
    } else if (token_arr[4] == NULL) {
      // Edit single specified command
      start = end = atoi(token_arr[3]);
    } else {
      // Edit range of commands
      start = atoi(token_arr[3]);
      end = atoi(token_arr[4]);
    }

    // Writing a specified range to temporary file
    FILE *temp = fopen(temp_file, "w");
    if (temp == NULL) {
      perror("Error creating temporary file");
      return 1;
    }
    read_lines_wo_no(MEMORY_HISTORY_FILE, start, end);
    fclose(temp);

    // Opening editor with temporary file
    snprintf(command, sizeof(command), "%s %s", editor, temp_file);
    system(command);

    // Reading edited commands from temporary file
    FILE *edited = fopen(temp_file, "r");
    if (edited == NULL) {
      perror("Error opening edited file");
      return 1;
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, edited) != -1) {
      // Removing newline character
      line[strcspn(line, "\n")] = 0;

      // Executing the edited command
      system(line);

      // Adding the executed command to history
      FILE *fp = fopen(MEMORY_HISTORY_FILE, "a");
      if (fp != NULL) {
        fprintf(fp, "%s\n", line);
        fclose(fp);
      }
    }

    fclose(edited);
    if (line) {
      free(line);
    }

    // Clean up temporary file
    remove(temp_file);

    break;
  }

  case 5: { // fc -s
    char *old_word = NULL;
    char *new_word = NULL;
    int line_number = -1;
    int total_lines = count_lines(MEMORY_HISTORY_FILE);

    // Parse arguments
    if (token_arr[2] != NULL) {
      if (strchr(token_arr[2], '=') != NULL) {
        // String replacement specified
        old_word = strtok(token_arr[2], "=");
        new_word = strtok(NULL, "=");
      } else {
        // Command number specified
        line_number = atoi(token_arr[2]);
      }
    }

    if (token_arr[3] != NULL &&
        strchr(token_arr[3], '=') != NULL) { // Needs to be checked with echo
      old_word = strtok(token_arr[3], "=");
      new_word = strtok(NULL, "=");
    }

    // If no command number specified, use the last command
    if (line_number == -1) {
      line_number = total_lines;
    }

    // Read the specified command from history
    FILE *history = fopen(MEMORY_HISTORY_FILE, "r");
    if (history == NULL) {
      perror("Error opening history file");
      return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int current_line = 0;

    while ((read = getline(&line, &len, history)) != -1) {
      current_line++;
      if (current_line == line_number) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        break;
      }
    }

    fclose(history);

    if (current_line != line_number) {
      printf("fc: no command found\n");
      free(line);
      return 1;
    }

    // Perform string replacement if specified
    if (old_word != NULL && new_word != NULL) {
      char *replaced_line =
          malloc(len * 2); // Allocate more space for potential expansion
      if (replaced_line == NULL) {
        perror("Memory allocation error");
        free(line);
        return 1;
      }

      char *pos = strstr(line, old_word);
      if (pos != NULL) {
        // Perform the replacement
        strncpy(replaced_line, line, pos - line);
        replaced_line[pos - line] = '\0';
        strcat(replaced_line, new_word);
        strcat(replaced_line, pos + strlen(old_word));

        free(line);
        line = replaced_line;
      } else {
        free(replaced_line);
      }
    }

    // Displaying the command
    printf("%s\n", line);

    // Executing the command
    int result = system(line);

    // Adding the executed command to history
    history = fopen(MEMORY_HISTORY_FILE, "a");
    if (history != NULL) {
      fprintf(history, "%s\n", line);
      fclose(history);
    }

    free(line);
    return result;
  }

  case 6: // -d option
  {
    FILE *fp1 = fopen(MEMORY_HISTORY_FILE, "r");

    if (fp1 == NULL) {
      perror("Error opening history files");
      if (fp1)
        fclose(fp1);
      return -1;
    }

    if (token_arr[2] == NULL) {
      printf("psh: history -d requires a line number\n");
      fclose(fp1);
      return 1;
    }

    // Count total lines in global history
    int total_lines = count_lines(MEMORY_HISTORY_FILE);
    // printf("Total lines %d\n", total_lines);

    // Parse the offset
    long offset = strtol(token_arr[2], NULL, 10);
    size_t line_to_remove;

    if (offset > 0) {
      line_to_remove = (size_t)offset;
    } else if (offset < 0) {
      // Handle negative offset
      if ((size_t)(-offset) > total_lines) {
        printf("psh: history -d: %ld: history position out of range\n", offset);
        fclose(fp1);
        return 1;
      }
      line_to_remove = total_lines + offset + 1;
    } else {
      printf("psh: history -d: 0: history position out of range\n");
      fclose(fp1);
      return 1;
    }

    // Check if line_to_remove is within range
    if (line_to_remove > total_lines) {
      printf("psh: history -d: %zu: history position out of range\n",
             line_to_remove);
      fclose(fp1);
      return 1;
    }

    // Remove the line the memory history file
    remove_line(MEMORY_HISTORY_FILE, line_to_remove);

    printf("Removed line %zu from history.\n", line_to_remove);

    fclose(fp1);
    break;
  }

  case 7:
    clear_session_history();
    break;

  case 8: // history -p !line_no
  {
    FILE *global_history = fopen(MEMORY_HISTORY_FILE, "r");
    FILE *session_history = fopen(SESSION_HISTORY_FILE, "r");

    if (!global_history || !session_history) {
      perror("Error opening history files");
      return 1;
    }

    // Skipping the "history" and "-p" tokens
    int i = 2;
    while (token_arr[i] != NULL) {
      char *expanded = expand_history(token_arr[i], global_history);
      if (!expanded) {
        expanded = expand_history(token_arr[i], session_history);
      }

      printf("%s\n", expanded);
      free(expanded);
      i++;
    }

    fclose(global_history);
    fclose(session_history);
    return 1;

    break;
  }

  default:
    printf("psh: history: option not implemented\n");
    break;
  }
  return 1;
}
