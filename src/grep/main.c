#include <getopt.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int error_t;
#define is_error(err) ((err != 0))

#define PATTERN_SIZE 10000

static const char grep_flags_list[] = "e:ivclnhsf:o";

typedef struct grep_flags_struct {
  bool pattern;  //        Шаблон                                 (флаг -e)
  bool ignore_reg_i;  //   Игнорирование регистра                 (флаг -i)
  bool invert_find_v;  //  Инвертивный поиск                      (флаг -v)
  bool count_match_c;  //  Количество совпавших строк             (флаг -c)
  bool only_files_l;  //   Вывод только файлалов                  (флаг -l)
  bool number_n;  //       Номер строки внутри файла              (флаг -n)
  bool hide_file_h;  //    Вывод без указания файла               (флаг -h)
  bool no_err_file_s;  //  Не выводит ошибки об отсутствии файла  (флаг -s)
  bool file_pattern_f;  // Шаблон внутри файла                    (флаг -f)
  bool only_find_o;  //    Соответствия без лишней части          (флаг -o)
  bool single_file;  //    При одном файле, нейминг не выводится
  bool skip_file;  //      Работает вместе с флагом -ov

} grep_flags_struct;

error_t get_active_flags(int argc, char** argv, char* patterns,
                         size_t max_pat_size, grep_flags_struct* grep_flags);
error_t add_str_pattern(char* str, size_t str_size, char* patterns,
                        size_t max_pat_size);
error_t add_file_pattern(const char* filename, char* patterns,
                         size_t max_pat_size);

error_t process_function(int argc, char** argv, char* patterns, int optind,
                         grep_flags_struct grep_flags);
error_t grep_function(FILE* file, const char* filename, char* patterns,
                      grep_flags_struct grep_flags);
error_t only_match_out(char* line, char* patterns, grep_flags_struct grep_flags,
                       const char* filename, int line_number);
bool check_match(char* line, char* patterns, bool ignore_reg_i, error_t* err);

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Ошибка количества аргументов\n");
    return EXIT_FAILURE;
  }

  error_t err = false;
  grep_flags_struct grep_flags = {false};
  char patterns[PATTERN_SIZE] = {0};  // Создаем массив паттернов

  // Получение активных параметров
  if (!is_error(err)) {
    err = get_active_flags(argc, argv, (char*)patterns, PATTERN_SIZE,
                           &grep_flags);
  }

  // Process функция
  if (!is_error(err)) {
    err = process_function(argc, argv, patterns, optind, grep_flags);
  }

  return (!is_error(err)) ? EXIT_SUCCESS : EXIT_FAILURE;
}

error_t get_active_flags(int argc, char** argv, char* patterns, size_t max_pat_size, grep_flags_struct* grep_flags) {
  error_t err = false;

  char flag_name;
  while (!is_error(err) &&
         ((flag_name = getopt(argc, argv, grep_flags_list)) != -1)) {
    switch (flag_name) {
      case 'e':
        grep_flags->pattern = true;
        err = add_str_pattern(optarg, strlen(optarg), patterns, max_pat_size);
        break;
      case 'i':
        grep_flags->ignore_reg_i = true;
        break;
      case 'v':
        grep_flags->invert_find_v = true;
        break;
      case 'c':
        grep_flags->count_match_c = true;
        break;
      case 'l':
        grep_flags->only_files_l = true;
        break;
      case 'n':
        grep_flags->number_n = true;
        break;
      case 'h':
        grep_flags->hide_file_h = true;
        break;
      case 's':
        grep_flags->no_err_file_s = true;
        break;
      case 'f':
        grep_flags->file_pattern_f = true;
        err = add_file_pattern(optarg, patterns, max_pat_size);
        break;
      case 'o':
        grep_flags->only_find_o = true;
        break;
      default:
        err = true;
        fprintf(stderr, "Ошибка ввода данных");
    }
  }

  if (!is_error(err)) {
    // grep без флагов
    if ((argc > optind) && !grep_flags->pattern &&
        !grep_flags->file_pattern_f) {
      err = add_str_pattern(argv[optind], strlen(argv[optind]), patterns,
                            max_pat_size);
      optind++;
    }

    // Функция приоритетов флагов
    // флаг l обнуляет c, o, n, h
    if (grep_flags->only_files_l) {
      grep_flags->count_match_c = false;
      grep_flags->only_find_o = false;
      grep_flags->number_n = false;
      grep_flags->hide_file_h = false;
    }

    // флаг с обнуляет работу флага о и n
    if (grep_flags->count_match_c) {
      grep_flags->number_n = false;
      grep_flags->only_find_o = false;
    }

    // Пропускаем проверку, с этими флагами в GNU grep выводит None
    if (grep_flags->invert_find_v && grep_flags->only_find_o) {
      grep_flags->skip_file = true;
    }

    if ((argc - optind) == 1) grep_flags->single_file = true;
  }

  return err;
}

error_t add_str_pattern(char* str, size_t str_size, char* patterns,
                        size_t max_pat_size) {
  error_t err = false;
  size_t pat_size = strlen(patterns);

  if (str_size + pat_size >= max_pat_size) {
    err = true;
    fprintf(stderr, "Превышен максимальный размер шаблона\n");
  }

  if (!is_error(err)) {
    if (pat_size == 0) {
      strcat(patterns, str);
    } else {
      strcat(patterns, "\\|");
      strcat(patterns, str);
    }
  }

  return err;
}

error_t add_file_pattern(const char* filename, char* patterns,
                         size_t max_pat_size) {
  error_t err = false;

  FILE* regex_file = fopen(filename, "r");
  if (regex_file == NULL) {
    err = true;
    fprintf(stderr, "Ошибка: { %s }: Файл шаблонов не определен\n", filename);
  }

  char* line = NULL;
  ssize_t line_len = 0;
  size_t line_size = 0;

  while (!is_error(err) &&
         ((line_len = getline(&line, &line_size, regex_file)) != -1)) {
    if (line[line_len - 1] == '\n') line[line_len - 1] = '\0';

    err = add_str_pattern(line, line_len, patterns, max_pat_size);
  }

  if (line != NULL) free(line);
  if (regex_file != NULL) fclose(regex_file);

  return err;
}

// ----------------------------------------------------------------------------
error_t process_function(int argc, char** argv, char* patterns, int optind,
                         grep_flags_struct grep_flags) {
  error_t err = false;

  for (int i = optind; !is_error(err) && i < argc; i++) {
    bool skip = false;
    const char* filename = argv[i];

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
      if (!grep_flags.no_err_file_s)
        fprintf(stderr, "Ошибка: { %s }: Файл не определен\n", filename);
      skip = true;
    }

    if (!skip) err = grep_function(file, filename, patterns, grep_flags);

    if (file == NULL) fclose(file);
  }

  return err;
}

error_t grep_function(FILE* file, const char* filename, char* patterns,
                      grep_flags_struct grep_flags) {
  error_t err = false;

  bool file_checked = false;
  int line_number = 0;
  int match_count = 0;
  int last_symbol = 0;

  char* line = NULL;
  ssize_t len = 0;
  size_t size = 0;

  while (!is_error(err) && !file_checked &&
         ((len = getline(&line, &size, file)) != EOF)) {
    line_number++;
    last_symbol = line[len - 1];

    // Проверка строки файла на совпадения
    bool is_match = check_match(line, patterns, grep_flags.ignore_reg_i, &err);

    // Обработка is_match
    if (!is_error(err)) {
      // Флаг -v
      if (grep_flags.invert_find_v) is_match = !is_match;

      // Увеличение счетчика match
      if (is_match) match_count++;

      // Если требуются только файлы
      if (is_match && grep_flags.only_files_l) {
        printf("%s\n", filename);
        file_checked = true;
      }
    }

    // Вывод строки
    if (!is_error(err) && is_match &&
        !(file_checked || grep_flags.count_match_c || grep_flags.skip_file ||
          grep_flags.only_files_l)) {
      // Вывод только найденных элементов
      if (grep_flags.only_find_o)
        err = only_match_out(line, patterns, grep_flags, filename, line_number);

      if (!grep_flags.only_find_o) {
        // Вывод filename если несколько файлов
        if (!grep_flags.single_file && !grep_flags.hide_file_h)
          printf("%s:", filename);

        // Вывод номера строки файла
        if (grep_flags.number_n) printf("%d:", line_number);

        // вывод строки
        printf("%s", line);
        if (last_symbol != '\n') putchar('\n');
      }
    }
  }

  // Если флаг -с
  if (!is_error(err) && grep_flags.count_match_c) {
    // Вывод названия файла, если файлов несколько.
    if (!grep_flags.single_file && !grep_flags.hide_file_h) {
      printf("%s:", filename);
    }

    printf("%d\n", match_count);
  }

  if (line != NULL) free(line);
  return err;
}

bool check_match(char* line, char* patterns, bool ignore_reg_i, error_t* err) {
  bool match = false;

  regex_t regex;
  *err = regcomp(&regex, patterns, (ignore_reg_i ? REG_ICASE : REG_NEWLINE));

  if (is_error(*err)) {
    fprintf(stderr, "Шаблон { %s }: Ошибка компиляции\n", patterns);
    ;
  } else {
    match = (regexec(&regex, line, 0, NULL, 0) == 0);
  }

  regfree(&regex);
  return match;
}

error_t only_match_out(char* line, char* patterns, grep_flags_struct grep_flags,
                       const char* filename, int line_number) {
  error_t err = false;
  regex_t regex;

  err = regcomp(&regex, patterns,
                (grep_flags.ignore_reg_i ? REG_ICASE : REG_NEWLINE));

  if (is_error(err)) {
    fprintf(stderr, "Ошибка компиляции файла %s\n", patterns);
  } else {
    regmatch_t match_elem;

    bool is_match = false;
    do {
      is_match = ((regexec(&regex, line, 1, &match_elem, 0)) == 0);

      if (grep_flags.invert_find_v) is_match = !is_match;

      if (is_match) {
        if (!grep_flags.single_file && !grep_flags.hide_file_h)
          printf("%s:", filename);

        if (grep_flags.number_n) printf("%d:", line_number);

        printf("%.*s\n", (int)(match_elem.rm_eo - match_elem.rm_so),
               line + match_elem.rm_so);

        line += match_elem.rm_eo;
      }
    } while (is_match);
  }

  regfree(&regex);

  return err;
}