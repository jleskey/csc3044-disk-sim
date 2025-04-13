/**
 * A disk seeking simulation
 *
 * @file main.c
 * @author Joseph Leskey <josleskey@mail.mvnu.edu>
 * @date 1 April 2025
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#define D_SIZE_MIN 0
#define D_SIZE_MAX 65535
#define D_POS_INIT 32767

#define D_DYNAMIC_BASE_SIZE 10

#define safe_malloc(size) _safe_malloc(size, __FILE__, __LINE__)
#define safe_realloc(ptr, size) _safe_realloc(ptr, size, __FILE__, __LINE__)

typedef struct SeekList
{
    int *list;
    int length;
} SeekList;

bool streq(const char *a, const char *b);
int randint(const int min, const int max);

void *_safe_malloc(const size_t size, const char *file, const int line);
void *_safe_realloc(void *ptr, const size_t size, const char *file,
                    const int line);

void printHeader(const char text[]);
void printIntList(const int list[], const int length);

SeekList generateRandomSeeks(const int number);
SeekList extractSeeks(FILE *stream);

void shortestSeekFirst(SeekList *seeks);
void elevatorAlgorithm(SeekList *seeks);

void printOverview(SeekList seeks);
void printRunStats(SeekList seeks, const char title[]);

void process(SeekList seeks);

int initialPosition = D_POS_INIT;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf(
            "Usage: %s <command>\n"
            "\n"
            "Commands:\n"
            "file <path>    –   read disk seeks from file at path\n"
            "in             –   read disk seeks from stdin\n"
            "rand <number>  –   use given number of random disk seeks\n",
            argv[0]);
    }
    else
    {
        const char *command = argv[1];

        SeekList seeks;

        if (streq(command, "file"))
        {
            if (argc < 3)
            {
                printf("Usage: %s file <path>\n", argv[0]);
                return EXIT_FAILURE;
            }
            else
            {
                const char *path = argv[2];

                FILE *file = fopen(path, "r");

                if (file != NULL)
                {
                    seeks = extractSeeks(file);
                }
                else
                {
                    fprintf(stderr, "Could not open file: %s\n", path);
                    return EXIT_FAILURE;
                }

                fclose(file);
            }
        }
        else if (streq(command, "in"))
        {
            seeks = extractSeeks(stdin);
        }
        else if (streq(command, "rand"))
        {
            if (argc < 3)
            {
                printf("Usage: %s rand <number>\n", argv[0]);
                return EXIT_FAILURE;
            }
            else
            {
                const int number = atoi(argv[2]);
                seeks = generateRandomSeeks(number);
            }
        }
        else
        {
            fprintf(stderr, "Unknown command: %s\n", command);
            return EXIT_FAILURE;
        }

        if (seeks.list != NULL)
        {
            process(seeks);
            free(seeks.list);
        }
        else
        {
            fprintf(stderr, "Failed to create list of disk seeks.\n");
        }
    }
}

bool streq(const char *a, const char *b)
{
    for (; *a != '\0'; a++, b++)
    {
        if (*b == '\0' || *a != *b)
            return false;
    }

    return *b == '\0';
}

SeekList generateRandomSeeks(const int number)
{
    int *seeks = safe_malloc(number * sizeof(int));

    srand(time(NULL));

    for (int i = 0; i < number; i++)
        seeks[i] = randint(D_SIZE_MIN, D_SIZE_MAX);

    return (SeekList){seeks, number};
}

SeekList extractSeeks(FILE *stream)
{
    int size = D_DYNAMIC_BASE_SIZE;
    int *seeks = safe_malloc(sizeof(int) * size);

    int seek;
    int number = 0;

    while (fscanf(stream, "%d\n", &seek) == 1)
    {
        if (number == size)
        {
            size *= 2;
            seeks = safe_realloc(seeks, sizeof(int) * size);
        }

        seeks[number++] = seek;
    }

    return (SeekList){seeks, number};
}

void process(SeekList seeks)
{
    // Starting position
    const char *initialPositionInput = getenv("D_POS_INIT");
    if (initialPositionInput != NULL)
    {
        initialPosition = atoi(initialPositionInput);
    }

    // Overview
    printOverview(seeks);

    // First come, first served algorithm
    printRunStats(seeks, "First come, first served");

    // Shortest seek first algorithm
    shortestSeekFirst(&seeks);
    printRunStats(seeks, "Shortest seek first");

    // Elevator algorithm
    elevatorAlgorithm(&seeks);
    printRunStats(seeks, "Elevator algorithm");

    // Comfortable padding
    printf("\n");
}

void printOverview(SeekList seeks)
{
    printHeader("Overview");

    int sum = 0;

    for (int i = 0; i < seeks.length; i++)
    {
        sum += seeks.list[i];
    }

    // Calculate mean.
    double mean = sum / (double)seeks.length;

    int sumOfDeviations = 0;

    // Calculate sum of deviations.
    for (int i = 0; i < seeks.length; i++)
    {
        sumOfDeviations += (seeks.list[i] - mean) * (seeks.list[i] - mean);
    }

    // Calculate variance.
    double variance = sumOfDeviations / (double)seeks.length;

    // Calculate standard deviation.
    double stddev = sqrt(variance);

    // Drop the stats.
    printf(
        "Initial position: %d\n"
        "Total seeks: %d\n"
        "Mean: %.4f\n"
        "Standard deviation: %.4f\n",
        initialPosition, seeks.length, mean, stddev);
}

void printRunStats(SeekList seeks, const char title[])
{
    printHeader(title);

    int distance = 0;
    int source = initialPosition;

    for (int i = 0; i < seeks.length; i++)
    {
        distance += abs(seeks.list[i] - source);
        source = seeks.list[i];
    }

    printf("Total distance: %d\n", distance);
    printf("\n");
    printIntList(seeks.list, seeks.length);
}

void shortestSeekFirst(SeekList *seeks)
{
    int currentPosition = initialPosition;

    for (int i = 0; i < seeks->length; i++)
    {
        int bestIndex = -1;
        int smallestDistance = INT_MAX;
        int nextPosition;

        for (int j = i; j < seeks->length; j++)
        {
            int position = seeks->list[j];
            int distance = abs(position - currentPosition);
            if (distance < smallestDistance)
            {
                smallestDistance = distance;
                bestIndex = j;
                nextPosition = position;
            }
        }

        if (bestIndex != -1)
        {
            if (bestIndex != i)
            {
                int currentValue = seeks->list[i];
                seeks->list[i] = seeks->list[bestIndex];
                seeks->list[bestIndex] = currentValue;
            }

            currentPosition = nextPosition;
        }
    }
}

void elevatorAlgorithm(SeekList *seeks)
{
    int currentPosition = initialPosition;

    bool up = true;

    for (int n = 0; n < 2; n++)
    {
        for (int i = 0; i < seeks->length; i++)
        {
            int bestIndex = -1;
            int smallestDistance = INT_MAX;

            for (int j = i; j < seeks->length; j++)
            {
                int position = seeks->list[j];

                if ((up && position >= currentPosition) ||
                    (!up && position <= currentPosition))
                {
                    int distance = abs(position - currentPosition);
                    if (distance < smallestDistance)
                    {
                        smallestDistance = distance;
                        bestIndex = j;
                        currentPosition = position;
                    }
                }
            }

            if (bestIndex != -1)
            {
                int currentValue = seeks->list[i];
                seeks->list[i] = seeks->list[bestIndex];
                seeks->list[bestIndex] = currentValue;
            }
        }

        up = !up;
    }
}

void printHeader(const char text[])
{
    printf("\n%s\n", text);
    for (int i = 0; text[i] != '\0'; i++)
    {
        printf("=");
    }
    printf("\n\n");
}

void printIntList(const int list[], const int length)
{
    for (int i = 0; i < length; i++)
    {
        printf("%d%s", list[i], i + 1 == length ? "\n" : ", ");
    }
}

int randint(const int min, const int max)
{
    // Let it be distinctly understood that this is both non-random and
    // potentially non-uniform, but it's probably good enough for this
    // use case.
    return min + rand() % (max - min + 1);
}

void *_safe_malloc(const size_t size, const char *file, const int line)
{
    void *_ptr = malloc(size);
    if (_ptr == NULL)
    {
        fprintf(stderr, "Allocation error @ %s:%d (%zu bytes)\n", file, line,
                size);
        exit(EXIT_FAILURE);
    }
    return _ptr;
}

void *_safe_realloc(void *ptr, const size_t size, const char *file,
                    const int line)
{
    void *_ptr = realloc(ptr, size);
    if (_ptr == NULL)
    {
        fprintf(stderr, "Allocation error @ %s:%d (%zu bytes)\n", file, line,
                size);
        free(ptr);
        exit(EXIT_FAILURE);
    }
    return _ptr;
}
