#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/**
 * Простой генератор случайных систем линейных уравнений
 * Создает случайную матрицу коэффициентов и вычисляет правую часть
 */

int main(int argc, char *argv[]) {
    // Проверка аргументов
    if (argc != 3) {
        printf("Использование: %s <размер_матрицы> <имя_файла>\n", argv[0]);
        printf("Пример: %s 5 system5.txt\n", argv[0]);
        return 1;
    }

    // Парсинг аргументов
    int n = atoi(argv[1]);
    const char *filename = argv[2];

    if (n < 2) {
        printf("Размер матрицы должен быть не менее 2\n");
        return 1;
    }

    // Инициализация генератора случайных чисел
    srand(time(NULL));

    // Выделение памяти
    double **matrix = (double **)malloc(n * sizeof(double *));
    double *true_x = (double *)malloc(n * sizeof(double));
    
    for (int i = 0; i < n; i++) {
        matrix[i] = (double *)malloc((n + 1) * sizeof(double));
    }

    // Генерация случайного истинного решения
    for (int i = 0; i < n; i++) {
        true_x[i] = (double)(rand() % 200 - 100) / 10.0; // от -10.0 до 10.0
    }

    // Генерация случайной матрицы коэффициентов
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = (double)(rand() % 200 - 100) / 10.0; // от -10.0 до 10.0
        }
        
        // Увеличиваем диагональные элементы для устойчивости системы
        matrix[i][i] += 15.0;
        
        // Вычисление правой части: b = A * x
        matrix[i][n] = 0.0;
        for (int j = 0; j < n; j++) {
            matrix[i][n] += matrix[i][j] * true_x[j];
        }
    }

    // Сохранение в файл
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Ошибка создания файла %s\n", filename);
        return 1;
    }

    // Запись размера системы
    fprintf(file, "%d\n", n);

    // Запись матрицы коэффициентов и правой части
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= n; j++) {
            fprintf(file, "%.6f ", matrix[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);

    // Освобождение памяти
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
    free(true_x);

    printf("Создана система %dx%d в файле %s\n", n, n, filename);

    return 0;
}