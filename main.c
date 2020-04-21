#include <stdio.h> //printf(3), perror(3)
#include <stdlib.h> //exit(3), malloc(3), free(3), realloc(3), atoi(3)
#include <fcntl.h> //open(2)
#include <unistd.h> //read(2), lseek(2), close(2)
#include <string.h> //strlen(3)
#include <ctype.h> //isdigit(3)

#define BUF_SIZE 1024
#define NUMBER_BUF_SIZE 11
#define MAX_INT_LENGTH 10
#define EXIT_CMD 0

#define ERR_MEMORY -1
#define ERR_OPEN -2
#define ERR_READ -3
#define ERR_CLOSE -4
#define ERR_INCORRECTNUM -5
#define ERR_SEEK -6
#define ERR_WRITE -7
#define ERR_NOTDIGIT -8
#define ERR_PRINT -9

typedef struct LineSeparatorTableEntry {
    int offset;
    int length;
} LSTEntry;

typedef struct LineSeparatorTable {
    LSTEntry *entries;
    int size;
    int maxSize;
} LSTable;

int addEntry(LSTable *table, int offset, int length) {
    LSTEntry entry;
    entry.offset = offset;
    entry.length = length;

    if (table->size >= table->maxSize) {
        table = realloc(table->entries, sizeof(LSTEntry) * (table->maxSize + BUF_SIZE));
        if (table == NULL) {
            perror("Cannot reallocate memory for table");
            return ERR_MEMORY;
        }
        table->maxSize += BUF_SIZE;
    }

    table->entries[table->size] = entry;
    table->size++;

    return 0;
}

int makeTable(char *fileName, LSTable *table) {
    int fd, t, err = 0;
    int offset, length;
    char c;
    fd = open(fileName, O_RDONLY);
    if(fd == -1) {
        perror("Cannot open file");
        return ERR_OPEN;
    }
    offset = 0; length = 0;
    while ((t = read(fd, &c, 1)) > 0) {
        offset++;
        length++;
        if (c == '\n') {
            if ((err = addEntry(table, offset - length, length)) < 0) {
                return err;
            }
            //printf("length: %d, offset: %d\n", length, offset);
            length = 0;
        }
    }
    if (t == -1) {
        perror("Error while reading from a file");
        return ERR_READ;
    }

    if (close(fd) == -1) {
        perror("Error while closing the file");
        return ERR_CLOSE;
    }

    return 0;
}

int printLine(char *fileName, LSTable *table, int lineNumber) {
    int fd, t, err;
    if (lineNumber > table->size || lineNumber < 0) {
        fprintf(stderr, "A line with the given number does not exist\n");
        return ERR_INCORRECTNUM;
    }
    if ((fd = open(fileName, O_RDONLY)) == -1) {
        perror("Cannot open file");
        return ERR_OPEN;
    }
    LSTEntry entry = table->entries[lineNumber - 1];
    char *buffer = calloc(sizeof(char), entry.length);
//    printf("Entry: length: %d, offset: %d\n", entry.length, entry.offset);
//    printf("Current buffer: %s\n", buffer);
    if (buffer == NULL) {
        perror("Cannot create a buffer to read lines");
        return ERR_MEMORY;
    }
    if ((err = lseek(fd, entry.offset, SEEK_SET)) == -1) {
        perror("Error while moving the cursor to the correct line");
        free(buffer);
        return ERR_SEEK;
    }
    if ((err = read(fd, buffer, entry.length)) == -1) {
        perror("Error while reading a file");
        free(buffer);
        return ERR_READ;
    }
    //printf("read: %d\n", err);
    //rest of the buffer gets filled with junk after reading when on Solaris
    if (strlen(buffer) > entry.length) {
        buffer[entry.length] = '\0';
    }
    if (close(fd) == -1) {
        perror("Error while closing the file");
        free(buffer);
        return ERR_CLOSE;
    }
    if (printf("%s", buffer) < 0) {
        perror("Printing error");
        free(buffer);
        return ERR_PRINT;
    }
    //printf("Buf size: %lu\n", strlen(buffer));

    free(buffer);

    return 0;
}

int readLineNumbers(int *number) {
    int err, i;
    char *msg = "Enter your number: ";
    if ((err = write(STDOUT_FILENO, msg, strlen(msg))) == - 1) {
        perror("Error while writing a message to enter the line");
        return ERR_WRITE;
    }

    char *result = calloc(sizeof(char), NUMBER_BUF_SIZE);
    if (result == NULL) {
        perror("Cannot create a buffer to read numbers");
        return ERR_MEMORY;
    }
    if ((err = read(STDIN_FILENO, result, NUMBER_BUF_SIZE)) == -1) {
        perror("Cannot allocate memory for number buffer");
        free(result);
        return ERR_READ;
    }

    if (err > MAX_INT_LENGTH) {
        fprintf(stderr, "Entered value is not a non-negative integer\n");
        free(result);
        return ERR_NOTDIGIT;
    }
    for (i = 0; i < err - 1; i++) {
        if (!isdigit(result[i])) {
            fprintf(stderr, "Entered value is not a non-negative integer\n");
            free(result);
            return ERR_NOTDIGIT;
        }
    }

    *number = atoi(result);

    free(result);

    return 0;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        char *msg = "Usage: ./program filename\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(1);
    }

    int err, num = 0;
    LSTEntry *entries = malloc(sizeof(LSTEntry) * BUF_SIZE);
    if (entries == NULL) {
        fprintf(stderr, "Cannot allocate memory for table entries\n");
        exit(1);
    }
    LSTable table;
    table.entries = entries;
    table.maxSize = BUF_SIZE;
    table.size = 0;

    if ((err = makeTable(argv[1], &table)) < 0) {
        free(table.entries);
        fprintf(stderr, "Error while creating a table, error code: %d\n", err);
        exit(2);
    }

    while (1) {
        if ((err = readLineNumbers(&num)) < 0) {
            fprintf(stderr, "Error while reading line numbers, error code: %d\n", err);
            break;
        }
        if (num == EXIT_CMD)
            break;
        if ((err = printLine(argv[1], &table, num)) < 0) {
            fprintf(stderr, "Error while printing a line, error code: %d\n", err);
            break;
        }
    }
    free(table.entries);
    if (err < 0) {
        exit(3);
    }

    return 0;
}
