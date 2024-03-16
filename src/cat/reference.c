#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef int error_code;
#define is_error(err) ((err != 0))

static const char SHORT_OPTIONS[] = "beEvnstTA";
static const struct option LONG_OPTIONS[] = {
    {"number-nonblank", no_argument, NULL, 'b'},
    {"number", no_argument, NULL, 'n'},
    {"squeeze-blank", no_argument, NULL, 's'},
    {"show-ends", no_argument, NULL, 'E'},
    {"show-tabs", no_argument, NULL, 'T'},
    {"show-all", no_argument, NULL, 'A'},
    {NULL, 0, NULL, 0}};

typedef struct cat_flags {
  bool number;
  bool number_nonblank; 
  bool squeeze_blank;
  bool show_ends;
  bool show_tabs;
  bool show_nonprinting;
} cat_flags;

typedef struct manage_data {
  int prev_ch;
  bool is_prev_empty;
  unsigned int line_cntr;
} manager_data;

void usage(const char *options);
error_code parse_args(int argc, char **argv, cat_flags *flags);
void process_files(int argc, char **argv, int arg_i, cat_flags flags);

int main(int argc, char **argv) {
  error_code err = false;

  cat_flags flags = {false};

  err = parse_args(argc, argv, &flags);

  if (!is_error(err)) {
    process_files(argc, argv, optind, flags);
  }

  return !is_error(err) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void usage(const char *options) {
  printf("Usage: ./s21_cat [-%s]... [FILE]...", options);
}

error_code parse_args(int argc, char **argv, cat_flags *flags) {
  error_code err = false;

  int arg;
  while (!is_error(err) && ((arg = getopt_long(argc, argv, SHORT_OPTIONS,
                                               LONG_OPTIONS, NULL)) != -1)) {
    switch (arg) {
      case 'A':
        flags->show_nonprinting = true;
        flags->show_tabs = true;
        flags->show_ends = true;
        break;

      case 'b':
        flags->number = true;
        flags->number_nonblank = true;
        break;

      case 'e':
        flags->show_ends = true;
        flags->show_nonprinting = true;
        break;

      case 'n':
        flags->number = true;
        break;

      case 's':
        flags->squeeze_blank = true;
        break;

      case 't':
        flags->show_tabs = true;
        flags->show_nonprinting = true;
        break;

      case 'v':
        flags->show_nonprinting = true;
        break;

      case 'E':
        flags->show_ends = true;
        break;

      case 'T':
        flags->show_tabs = true;
        break;

      default:
        err = true;
        usage(SHORT_OPTIONS);
    }
  }

  return err;
}

void output_np_ch(int ch) {
  if ((ch == '\t') || (ch == '\n') || (ch >= 32 && ch < 127)) {
    putchar(ch);
  } else if (ch == 127) {
    printf("^?");
  } else if (ch >= 128 + 32) {
    printf("M-");
    (ch < 255) ? printf("%c", ch - 128) : printf("^?");
  } else {
    (ch > 32) ? printf("M-^%c", ch - 64) : printf("^%c", ch + 64);
  }
}

void output_ch(int ch, manager_data *mgr, cat_flags flags) {
  if (flags.squeeze_blank && (mgr->prev_ch == '\n') && (ch == '\n') &&
      mgr->is_prev_empty) {
    return;
  }

  mgr->is_prev_empty = (mgr->prev_ch == '\n' && ch == '\n');

  if (((flags.number && !flags.number_nonblank) ||
       (flags.number_nonblank && ch != '\n')) &&
      mgr->prev_ch == '\n') {
    mgr->line_cntr++;
    printf("%6d\t", mgr->line_cntr);
  }

  if (flags.show_ends && ch == '\n') {
    printf("$");
  }

  if (flags.show_tabs && ch == '\t') {
    printf("^");
    ch = 'I';
  }

  if (flags.show_nonprinting) {
    output_np_ch(ch);
  } else {
    putchar(ch);
  }

  mgr->prev_ch = ch;
}

void cat(FILE *file_input, manager_data *mgr, cat_flags flags) {
  int ch;
  while ((ch = fgetc(file_input)) != EOF) {
    output_ch(ch, mgr, flags);
  }
}

void process_files(int argc, char **argv, int arg_i, cat_flags flags) {
  manager_data mgr = {.prev_ch = '\n', .is_prev_empty = false, .line_cntr = 0};

  for (int i = arg_i; i < argc; i++) {
    bool skip = false;
    FILE *file = fopen(argv[i], "r");
    if (file == NULL) {
      fprintf(stderr, "Warning: No such file %s\n", argv[i]);
      skip = true;
    }

    if (!skip) {
      cat(file, &mgr, flags);
    }

    if (file != NULL) fclose(file);
  }
}
