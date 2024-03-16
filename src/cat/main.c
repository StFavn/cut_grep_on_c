#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

typedef int error_t;
#define is_error(err) ((err != 0))

char small_cat_flags_list[] = "beEvnstTA";
static struct option const long_cat_flags_list[] = {
        {"number-nonblank", no_argument, NULL, 'b'},
        {"number", no_argument, NULL, 'n'},
        {"squeeze-blank", no_argument, NULL, 's'},
        {"show-nonprinting", no_argument, NULL, 'v'},
        {"show-ends", no_argument, NULL, 'E'},
        {"show-tabs", no_argument, NULL, 'T'},
        {"show-all", no_argument, NULL, 'A'},
        {NULL, 0, NULL, 0}
    };
typedef struct cat_struct {
    bool number_nonblank;  //  Нумерует только не пустые строки
    bool number;  //           Нумерует каждую строку
    bool squeeze_blank;  //    Сжимет несколько пустых строк
    bool show_ends;  //        Выводит '$' перед символом '\n'
    bool show_nonprinting;  // Выводит скрытые символы
    bool show_tabs;  //        Выводит табуляции
} cat_struct;
typedef struct data_struct {
    int prev_c;  //           Предыдущий символ
    int curr_c;  //           Текущий символ         
    int line_number;  //      Счетчик вывода
    bool line_printed;  //    Проверкавывода предыдущей линии
} data_struct;


error_t get_active_flags(int argc, char **argv, cat_struct* cat_flags);
void process_function(int argc, char **argv, int optind, cat_struct cat_flags);
void cat_function(FILE *file, data_struct* cat_data, cat_struct cat_flags);
void print_curr_c(data_struct* cat_data, cat_struct cat_flags);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Ошибка количества аргументов\n");
        return EXIT_FAILURE;
    }

    error_t err = false;
    cat_struct cat_flags = {false};

    if (!is_error(err)) {
        err = get_active_flags(argc, argv, &cat_flags);
    }
     
    if (!is_error(err)) {
        process_function(argc, argv, optind, cat_flags);
    }   

    return !is_error(err) ? EXIT_SUCCESS : EXIT_FAILURE;
}

error_t get_active_flags(int argc, char **argv, cat_struct* cat_flags) {
    error_t err = false;
    
    char flag_name;
    while ((!is_error(err))  &&
        (flag_name = getopt_long(argc, argv, 
                                 small_cat_flags_list, long_cat_flags_list, NULL)) != -1) {
        switch (flag_name) {
            case 'b':  //                             Нумерует только не пустые строки
                cat_flags->number_nonblank = true;
                break;
            case 'n':  //                             Нумерует все строки
                cat_flags->number = true;
                break;
            case 'e':  //                             Показывает окончания и скрытые символы
                cat_flags->show_ends = true;
                cat_flags->show_nonprinting = true;
                break;
            case 's':  //                             Стакает строки
                cat_flags->squeeze_blank = true;
                break;
            case 't':  //                             Показывает табы и скрытые символы
                cat_flags->show_tabs = true;
                cat_flags->show_nonprinting = true;
                break;
            case 'v':  //                             Показывает скрытые символы
                cat_flags->show_nonprinting = true;
                break;
            case 'E':  //                             Показывает окончания
                cat_flags->show_ends = true;
                break;
            case 'T':  //                             Показывает табы
                cat_flags->show_tabs = true;
                break;
            case 'A':  //                             Показывает табы, окончания, скрытые символы
                cat_flags->show_nonprinting = true;
                cat_flags->show_tabs = true;
                cat_flags->show_ends = true;
                break;
            default:
                err = true;
                fprintf(stderr, "Ошибка получения флагов\n");
        }
    }
    return err;
}

void process_function(int argc, char** argv, int optind, cat_struct cat_flags) {
    data_struct cat_data = {
        .prev_c = '\n',
        .line_number = 0,
        .line_printed = false
    };
    
    for (int i = optind; i < argc; i++) {
        bool skip_file = false;

        FILE *file = fopen(argv[i], "r");
        if (file == NULL) {
            fprintf(stderr, "Файл { %s }: Ошибка чтения файла\n", argv[i]);
            skip_file =true;
        }

        if (!skip_file) {
            cat_function(file, &cat_data, cat_flags);
        }

        if (file != NULL) fclose(file);
    }
}

void cat_function(FILE *file, data_struct* cat_data, cat_struct cat_flags) {
    while ((cat_data->curr_c = fgetc(file)) != EOF) {
        print_curr_c(cat_data, cat_flags);
    }
}

void print_curr_c(data_struct* cat_data, cat_struct cat_flags)  {
     // Стакование строк (флаг -s)
    if (cat_flags.squeeze_blank  && 
        cat_data->prev_c == '\n' && 
        cat_data->curr_c == '\n' && 
        cat_data->line_printed) return;

    // Если передыдущий перенос, тогда пустая строка уже выведена
    cat_data->line_printed =
        (cat_data->prev_c == '\n' && cat_data->curr_c == '\n');

     // Нумерация только не пустых строк (флаг -b)
    if (cat_flags.number_nonblank &&
        cat_data->prev_c == '\n' && 
        cat_data->curr_c != '\n') {
            cat_data->line_number++;
            printf("%6d\t", cat_data->line_number);
    }

    // Нумерация всех строк (флаг -n)
    if (cat_flags.number && 
       !cat_flags.number_nonblank && 
        cat_data->prev_c == '\n') {
            cat_data->line_number += 1;
            printf("%6d\t", cat_data->line_number);
    }
   
    // Показать окончание строк (флаг -E)
    if (cat_flags.show_ends && 
        cat_data->curr_c == '\n') 
        printf("$");

    // Показать табуляции (флаг -T)
    if (cat_flags.show_tabs && 
        cat_data->curr_c == '\t') {
            printf("^");
            cat_data->curr_c = 'I';
        }

    // Показать скрытые символы (флаг -v)
    if (cat_flags.show_nonprinting) {
        if (cat_data->curr_c == '\t' || 
            cat_data->curr_c == '\n' ||
           (cat_data->curr_c >= 32 && 
            cat_data->curr_c < 127))
            putchar(cat_data->curr_c);
        else if (cat_data->curr_c == 127)
            printf("^?");
        else if (cat_data->curr_c >= 160) {
            printf("^?");
            (cat_data->curr_c < 255) ? 
            printf("%c", cat_data->curr_c - 128) : printf("^?");
        } else {
            (cat_data->curr_c > 32) ? 
            printf("M-^%c", cat_data->curr_c - 64) : 
            printf("^%c", cat_data->curr_c + 64);
        }
    } else {
        putchar(cat_data->curr_c);
    }
    
    cat_data->prev_c = cat_data->curr_c;
}