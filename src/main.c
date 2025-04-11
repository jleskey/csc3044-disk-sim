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
#define D_POS_CUR 32767

#define D_DYNAMIC_BASE_SIZE 10

typedef struct SeekList
{
    int *list;
    int length;
} SeekList;

bool streq(const char *a, const char *b);
int randint(const int min, const int max);

void printHeader(const char text[]);
void printIntList(const int list[], const int length);

SeekList generateRandomSeeks(const int number);
SeekList extractSeeks(FILE *stream);

void firstComeFirstServed(SeekList *seeks);
void shortestSeekFirst(SeekList *seeks);
void elevatorAlgorithm(SeekList *seeks);

void printStats(SeekList seeks, const char title[]);

void process(SeekList seeks);

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
    int *seeks = malloc(number * sizeof(int));

    srand(time(NULL));

    if (seeks != NULL)
    {
        for (int i = 0; i < number; i++)
            seeks[i] = randint(D_SIZE_MIN, D_SIZE_MAX);
    }
    else
    {
        fprintf(stderr, "Allocation error @ generateRandomSeek::seeks\n");
    }

    return (SeekList){seeks, number};
}

SeekList extractSeeks(FILE *stream)
{
    int _seeks_size = D_DYNAMIC_BASE_SIZE;
    int *seeks = malloc(sizeof(int) * _seeks_size);
    int number = 0;

    int seek;
    int i = 0;

    while (fscanf(stream, "%d\n", &seek) == 1)
    {
        if (i == number)
        {
            number *= 2;
            int *_seeks = realloc(seeks, sizeof(int) * number);
            if (_seeks)
            {
                seeks = _seeks;
            }
            else
            {
                free(seeks);
                fprintf(stderr, "Allocation error @ extractSeeks::_seeks");
                exit(EXIT_FAILURE);
            }
        }

        seeks[i++] = seek;
    }

    return (SeekList){seeks, i};
}

void process(SeekList seeks)
{
    // First come, first served algorithm
    printStats(seeks, "First come, first served");
    printIntList(seeks.list, seeks.length);

    // Shortest seek first algorithm
    shortestSeekFirst(&seeks);
    printStats(seeks, "Shortest seek first");
    printIntList(seeks.list, seeks.length);

    // Elevator algorithm
    elevatorAlgorithm(&seeks);
    printStats(seeks, "Elevator algorithm");
    printIntList(seeks.list, seeks.length);
}

void printStats(SeekList seeks, const char title[])
{
    printHeader(title);

    int distance = 0;
    int sum = seeks.list[0];

    // Calculate distance and sum.
    for (int i = 0; i < seeks.length - 1; i++)
    {
        int source = i == 0 ? D_POS_CUR : seeks.list[i];
        distance += abs(seeks.list[i] - source);
        sum += seeks.list[i + 1];
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
        "Distance: %d\n"
        "Mean: %.4f\n"
        "Standard deviation: %.4f\n"
        "\n",
        distance, mean, stddev);
}

void shortestSeekFirst(SeekList *seeks)
{
    int currentPosition = D_POS_CUR;

    for (int i = 0; i < seeks->length; i++)
    {
        int bestIndex = -1;
        int smallestDistance = INT_MAX;

        for (int j = i + 1; j < seeks->length; j++)
        {
            int position = seeks->list[j];
            int distance = abs(position - currentPosition);
            if (distance < smallestDistance)
            {
                smallestDistance = distance;
                bestIndex = j;
                currentPosition = position;
            }
        }

        if (bestIndex != -1)
        {
            int currentValue = seeks->list[i];
            seeks->list[i] = seeks->list[bestIndex];
            seeks->list[bestIndex] = currentValue;
        }
    }
}

void elevatorAlgorithm(SeekList *seeks)
{
    int currentPosition = D_POS_CUR;

    bool up = true;

    for (int n = 0; n < 2; n++)
    {
        for (int i = 0; i < seeks->length; i++)
        {
            int bestIndex = -1;
            int smallestDistance = INT_MAX;

            for (int j = i + 1; j < seeks->length; j++)
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
    printf("%s\n", text);
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
        printf("%d%s", list[i], i + 1 == length ? "\n\n" : ", ");
    }
}

int randint(const int min, const int max)
{
    // Let it be distinctly understood that this is both non-random and
    // potentially non-uniform, but it's probably good enough for this
    // use case.
    return min + rand() % (max - min + 1);
}
