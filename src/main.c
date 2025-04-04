/**
 * A disk simulator
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

SeekList generateRandomSeeks(const int number);
SeekList extractSeeks(FILE *stream);

void firstComeFirstServed(SeekList *seeks);
void shortestSeekFirst(SeekList *seeks);
void elevatorAlgorithm(SeekList *seeks);

void printStats(SeekList seeks);

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
    firstComeFirstServed(&seeks);
    printStats(seeks);
}

void printStats(SeekList seeks)
{
    int distance = 0;
    int sum = seeks.list[0];

    // Calculate distance and sum.
    for (int i = 0; i < seeks.length - 1; i++)
    {
        distance += abs(seeks.list[i + 1] - seeks.list[i]);
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

void firstComeFirstServed(SeekList *seeks)
{
    // We're assuming that the seeks are already in this order, so there
    // is nothing to do here. However, for the sake of semantics, we'll
    // assume that there could be.
    return;
}

void shortestSeekFirst(SeekList *seeks)
{
}

void elevatorAlgorithm(SeekList *seeks)
{
}

int randint(const int min, const int max)
{
    // Let it be distinctly understood that this is both non-random and
    // potentially non-uniform, but it's probably good enough for this
    // use case.
    return min + rand() % (max - min + 1);
}
