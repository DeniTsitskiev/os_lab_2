#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>  // Добавлен для gettimeofday()

#define MAX_THREADS 16

// Глобальные переменные для доступа из всех потоков
int thread_count;    // Количество потоков (задается пользователем)
int n;              // Размер системы уравнений
double **matrix;    // Расширенная матрица системы (n x n+1)
double *x;          // Вектор решения

// Структура для передачи данных в потоки
typedef struct {
    int start_row;  // Начальная строка для обработки
    int end_row;    // Конечная строка (не включая)
    int k;          // Текущий шаг исключения
} thread_data;

/**
 * Вывод матрицы на экран
 * mat - указатель на матрицу
 * size - размер матрицы (количество строк)
 */
void print_matrix(double **mat, int size) {
    printf("Расширенная матрица системы:\n");
    for (int i = 0; i < size; i++) {
        printf("Строка %2d: ", i);
        for (int j = 0; j <= size; j++) {
            if (j == size) {
                printf(" | ");  // Разделитель для правой части
            }
            printf("%8.3f ", mat[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * Вывод вектора решения
 * x - вектор решения
 * size - размер вектора
 */
void print_solution(double *x, int size) {
    printf("Решение системы:\n");
    for (int i = 0; i < size; i++) {
        printf("x[%2d] = %12.8f\n", i, x[i]);
    }
}

/**
 * Функция, выполняемая каждым потоком в прямом ходе метода Гаусса
 * arg - указатель на структуру thread_data с параметрами
 */
void *forward_elimination_thread(void *arg) {
    thread_data *data = (thread_data *)arg;
    int k = data->k;
    
    // Обрабатываем назначенные строки
    for (int i = data->start_row; i < data->end_row; i++) {
        if (i <= k) continue;  // Пропускаем уже обработанные строки
        
        // Вычисляем множитель для исключения элемента matrix[i][k]
        double factor = matrix[i][k] / matrix[k][k];
        
        // Вычитаем k-ю строку из i-й строки
        for (int j = k; j <= n; j++) {
            matrix[i][j] -= factor * matrix[k][j];
        }
    }
    
    pthread_exit(NULL);
}

/**
 * Прямой ход метода Гаусса с использованием потоков
 * Преобразует матрицу к верхнетреугольному виду
 */
void forward_elimination() {
    printf("Начало прямого хода метода Гаусса...\n");
    
    // Проходим по всем строкам кроме последней
    for (int k = 0; k < n - 1; k++) {
        // Проверка на нулевой диагональный элемент
        if (fabs(matrix[k][k]) < 1e-12) {
            // Поиск строки с ненулевым элементом в k-м столбце
            int swap_row = -1;
            for (int i = k + 1; i < n; i++) {
                if (fabs(matrix[i][k]) > 1e-12) {
                    swap_row = i;
                    break;
                }
            }
            
            if (swap_row == -1) {
                printf("Ошибка: система вырождена на шаге %d!\n", k);
                exit(1);
            }
            
            // Меняем строки местами
            printf("Перестановка строк %d и %d\n", k, swap_row);
            double *temp = matrix[k];
            matrix[k] = matrix[swap_row];
            matrix[swap_row] = temp;
        }
        
        // Если используется только один поток, выполняем последовательно
        if (thread_count == 1) {
            for (int i = k + 1; i < n; i++) {
                double factor = matrix[i][k] / matrix[k][k];
                for (int j = k; j <= n; j++) {
                    matrix[i][j] -= factor * matrix[k][j];
                }
            }
        } else {
            // Многопоточная обработка
            
            // Вычисляем сколько строк будет обрабатывать каждый поток
            int total_rows = n - k - 1;  // Всего строк для обработки
            int rows_per_thread = total_rows / thread_count;
            int remaining_rows = total_rows % thread_count;
            
            // Создаем массивы для потоков и их параметров
            pthread_t *threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
            thread_data *thread_args = (thread_data *)malloc(thread_count * sizeof(thread_data));
            
            int current_row = k + 1;
            for (int i = 0; i < thread_count; i++) {
                // Назначаем диапазон строк для текущего потока
                thread_args[i].start_row = current_row;
                thread_args[i].end_row = current_row + rows_per_thread + (i < remaining_rows ? 1 : 0);
                thread_args[i].k = k;
                
                // Создаем поток
                pthread_create(&threads[i], NULL, forward_elimination_thread, &thread_args[i]);
                
                // Переходим к следующему диапазону
                current_row = thread_args[i].end_row;
            }
            
            // Ожидаем завершения всех потоков
            for (int i = 0; i < thread_count; i++) {
                pthread_join(threads[i], NULL);
            }
            
            // Освобождаем память
            free(threads);
            free(thread_args);
        }
    }
    
    printf("Прямой ход завершен.\n");
}

/**
 * Обратный ход метода Гаусса
 * Находит решение из верхнетреугольной матрицы
 */
void back_substitution() {
    printf("Начало обратного хода...\n");
    
    // Проходим от последнего уравнения к первому
    for (int i = n - 1; i >= 0; i--) {
        // Начинаем с правой части уравнения
        x[i] = matrix[i][n];
        
        // Вычитаем известные компоненты решения
        for (int j = i + 1; j < n; j++) {
            x[i] -= matrix[i][j] * x[j];
        }
        
        // Делим на диагональный элемент
        x[i] /= matrix[i][i];
    }
    
    printf("Обратный ход завершен.\n");
}

/**
 * Загрузка системы уравнений из файла
 * filename - имя файла с матрицей
 * Возвращает 1 при успехе, 0 при ошибке
 */
int load_system_from_file(const char *filename) {
    printf("Загрузка системы из файла: %s\n", filename);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Ошибка: не удалось открыть файл %s\n", filename);
        return 0;
    }
    
    // Чтение размера системы
    if (fscanf(file, "%d", &n) != 1) {
        printf("Ошибка: неверный формат файла (размер системы)\n");
        fclose(file);
        return 0;
    }
    
    printf("Размер системы: %dx%d\n", n, n);
    
    // Выделение памяти для матрицы и вектора решения
    matrix = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++) {
        matrix[i] = (double *)malloc((n + 1) * sizeof(double));
    }
    x = (double *)malloc(n * sizeof(double));
    
    // Чтение матрицы коэффициентов и правой части
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= n; j++) {
            if (fscanf(file, "%lf", &matrix[i][j]) != 1) {
                printf("Ошибка чтения элемента матрицы [%d][%d]\n", i, j);
                fclose(file);
                return 0;
            }
        }
    }
    
    fclose(file);
    printf("Система успешно загружена из файла.\n");
    return 1;
}

/**
 * Вывод справки по использованию программы
 */
void print_help() {
    printf("\n=== Решатель систем линейных уравнений методом Гаусса ===\n\n");
    printf("Использование:\n");
    printf("  %s <файл_матрицы> <количество_потоков>\n", "main_solve");
    printf("\nАргументы:\n");
    printf("  файл_матрицы    - файл с матрицей системы (создается generate_matrix)\n");
    printf("  количество_потоков - число потоков для параллельных вычислений (1-%d)\n", MAX_THREADS);
    printf("\nПримеры:\n");
    printf("  %s system5.txt 1\n", "main_solve");
    printf("  %s system10.txt 4\n", "main_solve");
    printf("\nДля генерации матриц используйте generate_matrix:\n");
    printf("  generate_matrix 5 system5.txt 1\n");
    printf("  generate_matrix 10 system10.txt 2\n");
}

/**
 * Получение текущего времени в секундах с высокой точностью
 * Используется для измерения настенного времени
 */
double get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        // Обработка ошибки, если gettimeofday не сработал
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * 1e-6;
}

/**
 * Главная функция программы
 * Обрабатывает аргументы командной строки, управляет вычислительным процессом
 */
int main(int argc, char *argv[]) {
    // Обработка аргументов командной строки
    if (argc != 3) {
        print_help();
        return 1;
    }
    
    const char *filename = argv[1];
    thread_count = atoi(argv[2]);
    
    // Проверка корректности количества потоков
    if (thread_count < 1 || thread_count > MAX_THREADS) {
        printf("Ошибка: количество потоков должно быть от 1 до %d\n", MAX_THREADS);
        return 1;
    }
    
    printf("=== Параллельное решение СЛУ методом Гаусса ===\n");
    printf("Файл с матрицей: %s\n", filename);
    printf("Количество потоков: %d\n", thread_count);
    
    // Шаг 1: Загрузка системы уравнений из файла
    if (!load_system_from_file(filename)) {
        return 1;
    }
    
    // Шаг 2: Вывод исходной системы (только для небольших матриц)
    if (n <= 10) {
        printf("\n");
        print_matrix(matrix, n);
    } else {
        printf("\nМатрица слишком велика для отображения (n=%d)\n", n);
    }
    
    // Шаг 3: Замер времени начала вычислений (настенное время)
    double start_time = get_wall_time();
    
    // Шаг 4: Прямой ход метода Гаусса (может быть параллельным)
    forward_elimination();
    
    // Шаг 5: Вывод матрицы после прямого хода (для небольших систем)
    if (n <= 10) {
        printf("\nМатрица после прямого хода:\n");
        print_matrix(matrix, n);
    }
    
    // Шаг 6: Обратный ход метода Гаусса (последовательный)
    back_substitution();
    
    // Шаг 7: Замер времени окончания вычислений
    double end_time = get_wall_time();
    double execution_time = end_time - start_time;
    
    // Шаг 8: Вывод результатов
    printf("\n=== РЕЗУЛЬТАТЫ ===\n");
    if (n <= 20) {
        print_solution(x, n);
    } else {
        printf("Решение найдено (первые 10 компонент):\n");
        for (int i = 0; i < 10 && i < n; i++) {
            printf("x[%2d] = %12.8f\n", i, x[i]);
        }
        printf("... (всего %d компонент)\n", n);
    }
    
    // Шаг 9: Вывод информации о производительности
    printf("\n=== ПРОИЗВОДИТЕЛЬНОСТЬ ===\n");
    printf("Время выполнения: %.6f секунд\n", execution_time);
    printf("Размер системы: %d уравнений\n", n);
    printf("Использовано потоков: %d\n", thread_count);
    
    
    // Шаг 10: Освобождение памяти
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
    free(x);
    
    printf("\nПрограмма завершена успешно!\n");
    
    return 0;
}