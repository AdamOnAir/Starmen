#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 16  // Number of bytes per line
#define MAX_HISTORY 100  // Maximum number of undo actions

typedef struct {
    long offset;
    unsigned char old_value;
    unsigned char new_value;
} EditHistory;

void print_hex_line(unsigned char *buffer, int length, long offset) {
    printf("%08lx  ", offset);

    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (i < length)
            printf("%02x ", buffer[i]);
        else
            printf("   ");
    }

    printf(" |");
    for (int i = 0; i < length; i++) {
        if (isprint(buffer[i]))
            putchar(buffer[i]);
        else
            putchar('.');
    }
    printf("|\n");
}

void display_file(FILE *file, long file_size, long start_offset) {
    unsigned char buffer[BUFFER_SIZE];
    long offset = start_offset;
    int bytes_read;

    fseek(file, start_offset, SEEK_SET);

    while (offset < file_size && (bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        print_hex_line(buffer, bytes_read, offset);
        offset += bytes_read;
    }
}

void modify_byte(FILE *file, long offset, unsigned char new_value, EditHistory *history, int *history_count) {
    unsigned char old_value;
    fseek(file, offset, SEEK_SET);
    fread(&old_value, 1, 1, file);

    if (old_value == new_value) return; // No change needed

    fseek(file, offset, SEEK_SET);
    fwrite(&new_value, 1, 1, file);

    // Record history for undo
    if (*history_count < MAX_HISTORY) {
        history[*history_count].offset = offset;
        history[*history_count].old_value = old_value;
        history[*history_count].new_value = new_value;
        (*history_count)++;
    }
}

void undo_last_edit(FILE *file, EditHistory *history, int *history_count) {
    if (*history_count <= 0) {
        printf("No actions to undo.\n");
        return;
    }

    (*history_count)--;
    EditHistory last_edit = history[*history_count];
    fseek(file, last_edit.offset, SEEK_SET);
    fwrite(&last_edit.old_value, 1, 1, file);
    printf("Undo: Offset %08lx, restored %02x\n", last_edit.offset, last_edit.old_value);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb+");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    EditHistory history[MAX_HISTORY];
    int history_count = 0;

    long offset = 0;
    char command[20];
    unsigned int new_value;

    while (1) {
        system("clear");
        printf("Hex dump of %s (size: %ld bytes):\n\n", argv[1], file_size);
        display_file(file, file_size, offset);

        printf("\nCommands: \n");
        printf("  goto <hex_offset> - Go to offset\n");
        printf("  edit <hex_offset> <hex_value> - Edit byte\n");
        printf("  undo - Undo last edit\n");
        printf("  save - Save changes\n");
        printf("  exit - Exit without saving\n");
        printf("\nEnter command: ");

        fgets(command, sizeof(command), stdin);
        char *cmd = strtok(command, " ");

        if (strcmp(cmd, "goto") == 0) {
            char *hex_offset = strtok(NULL, " ");
            offset = strtol(hex_offset, NULL, 16);
            if (offset >= file_size) {
                printf("Offset out of bounds.\n");
                offset = 0;
            }
        } else if (strcmp(cmd, "edit") == 0) {
            char *hex_offset = strtok(NULL, " ");
            char *hex_value = strtok(NULL, " ");
            offset = strtol(hex_offset, NULL, 16);
            new_value = strtoul(hex_value, NULL, 16);

            if (offset < file_size && new_value <= 0xFF) {
                modify_byte(file, offset, (unsigned char)new_value, history, &history_count);
            } else {
                printf("Invalid offset or value.\n");
            }
        } else if (strcmp(cmd, "undo") == 0) {
            undo_last_edit(file, history, &history_count);
        } else if (strcmp(cmd, "save") == 0) {
            printf("Changes saved.\n");
            break;
        } else if (strcmp(cmd, "exit") == 0) {
            printf("Exiting without saving.\n");
            break;
        } else {
            printf("Unknown command.\n");
        }
    }

    fclose(file);
    return 0;
}

