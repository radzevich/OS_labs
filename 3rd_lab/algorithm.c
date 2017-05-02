#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 8
#define TRUE 1
#define FALSE 0

struct period
{
    char val;
    long len;
    long count;
};

struct period_set
{
    struct period prd_table[BLOCK_SIZE / 2 + 1];
    int count;
};

struct period_list
{
    struct period *prd_table;
    int count;
    int size;
};

typedef struct period_set prd_set;
typedef struct period_list prd_list;


void processValue(prd_set *p_set, char val);
void setValue(prd_set *p_set, char prd_val, long prd_len);
void saveToFile(prd_set *p_set, char *path);
void readFromFile(char *buf, size_t bytes_to_read, FILE *file);
void countPeriods(char *path);
void setEndPointValue(prd_set *p_set, char prd_val, long prd_len);
void addToPeriodList(prd_list *p_list, struct period *prd);
void processData(prd_list *p_list, char *data, long data_size);

prd_set prd_array[1 << BLOCK_SIZE];

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("error: to few arguments\n");
        exit(1);
    }

    memset((char*)prd_array, 0, sizeof(prd_array));

    for (int i = 0; i < 1 << BLOCK_SIZE; i++)
    {
        processValue(&(prd_array[i]), i);
    }

    //saveToFile(prd_array, argv[1]);
    //readFromFile((char*)prd_array, 256 * sizeof(*prd_array), argv[1]);
/*
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < BLOCK_SIZE / 2 + 1; j ++)
            printf("%d  val: %d  len: %ld  count: %ld\n", i, prd_array[i].prd_table[j].val, prd_array[i].prd_table[j].len, prd_array[i].prd_table[j].count);
        printf("mainCount %d\n\n", prd_array[i].count);
    }
*/
    countPeriods(argv[1]);

    return 0;
}


void processValue(prd_set *p_set, char val)
{
    char prev_bit_val = -1, cur_bit_val;
    int prd_len, shift;

    shift = BLOCK_SIZE - 1;
    prev_bit_val = ((1 << shift) & val) >> shift;
    shift--;
    prd_len = 1;

    while (shift >= 0)
    {
        cur_bit_val = ((1 << shift) & val) >> shift;

        if (cur_bit_val == prev_bit_val)
        {
            prd_len++;
        }
        else
        {
            if (p_set->count == 0)
                setEndPointValue(p_set, prev_bit_val, prd_len);
            else
                setValue(p_set, prev_bit_val, prd_len);

            prd_len = 1;
        }

        if (0 == shift)
        {
            setEndPointValue(p_set, cur_bit_val, prd_len);
        }

        prev_bit_val = cur_bit_val;
        shift--;
    }
}


void setEndPointValue(prd_set *p_set, char prd_val, long prd_len)
{
    p_set->prd_table[p_set->count].val = prd_val;
    p_set->prd_table[p_set->count].len = prd_len;
    p_set->prd_table[p_set->count].count = 1;
    p_set->count++;

}


void setValue(prd_set *p_set, char prd_val, long prd_len)
{
    for (int i = 1; i < p_set->count; i++)
    {
        if ((p_set->prd_table[i].val == prd_val) && (p_set->prd_table[i].len == prd_len))
        {
            p_set->prd_table[i].count++;
            return;
        }
    }

    p_set->prd_table[p_set->count].val = prd_val;
    p_set->prd_table[p_set->count].len = prd_len;
    p_set->prd_table[p_set->count].count = 1;
    p_set->count++;
}


void saveToFile(prd_set *p_set, char *path)
{
    FILE *file;
    size_t items_written, total_written;

    file = fopen(path, "w");
    if (NULL == file)
    {
        printf("error: opening file\n");
        exit(1);
    }

    //Saving results in file.
    total_written = 0;

    while (1)
    {
        items_written = fwrite(p_set + total_written, sizeof(prd_set), p_set->count - total_written, file);

        if (items_written <= 0)
            break;

        total_written += items_written;
    }

    fclose(file);
}


void readFromFile(char *buf, size_t bytes_to_read, FILE *file)
{
    size_t bytes_readen, total_readen;

    total_readen = 0;

    while (1)
    {
        bytes_readen = fread(buf + total_readen, sizeof(*buf), bytes_to_read - total_readen, file);

        if (bytes_readen <= 0)
            break;

        total_readen += bytes_readen;
    }
}


long getFileSize(FILE *file)
{
    long cur_pos, start_pos, end_pos;

    cur_pos = ftell(file);
    fseek(file, 0, SEEK_SET);
    start_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    end_pos = ftell(file);
    fseek(file, cur_pos, SEEK_SET);

    return end_pos - start_pos;
}


void countPeriods(char *path)
{
    FILE *file;
    long file_size;
    char *buf;
    prd_list *p_list;

    file = fopen(path, "r");
    if (NULL == file)
    {
        printf("error: opening file\n");
        exit(1);
    }

    file_size = getFileSize(file);

    buf = (char*)calloc(file_size, sizeof(char));
    p_list = (prd_list *)calloc(1, sizeof(prd_list));
    p_list->size = BLOCK_SIZE;
    p_list->prd_table = (struct period *)calloc(BLOCK_SIZE, sizeof(struct period));

    readFromFile(buf, file_size, file);
    processData(p_list, buf, file_size);

    for (int i = 0; i < p_list->count; i++)
    {
    	printf("val: %d  len: %ld  count: %ld\n", p_list->prd_table[i].val, p_list->prd_table[i].len, p_list->prd_table[i].count);
    }

    fclose(file);
}


void addToPeriodList(prd_list *p_list, struct period *prd)
{
     for (int i = 0; i < p_list->count; i++)
    {
        if ((p_list->prd_table[i].val == prd->val) && (p_list->prd_table[i].len == prd->len))
        {
            p_list->prd_table[i].count += prd->count;
            return;
        }
    }

    if (p_list->count == p_list->size)
    {

        p_list->size *= 2;
        p_list->prd_table = (struct period *)realloc(p_list->prd_table, p_list->size);
        if (p_list == NULL)
        {
            printf("error allcating period_list\n");
            exit(1);
        }
    }

    p_list->prd_table[p_list->count].val = prd->val;
    p_list->prd_table[p_list->count].len = prd->len;
    p_list->prd_table[p_list->count].count = prd->count;
    p_list->count++;
}


long checkNext(char *data, char val, long data_size)
{
    long index = 0;
    prd_set *prd;

    while (index < data_size)
    {
        prd = &prd_array[(unsigned char)data[index]];

        if ((prd->prd_table[0].val == val) && (prd->count == 1)) {
            index++;
        }
        else
            break;
    }

    return index;
}


void processData(prd_list *p_list, char *data, long data_size)
{
    long index = 0, repeats = 0, prev_processed = 0, i;
    long prev_len = 0;
    char val;
    struct period prd;


    while (index < data_size)
    {
        val = prd_array[(unsigned char)data[index]].prd_table[0].val;
        repeats = checkNext(&data[(unsigned char)index], val, data_size - index - 1);
        prev_processed = 0;
        index += repeats;
        prd.val = val;
        prd.len = prev_len + (repeats) * BLOCK_SIZE;
        prd.count = 1;

        if (val == prd_array[(unsigned char)data[index]].prd_table[0].val) {
            prd.len += prd_array[(unsigned char)data[index]].prd_table[0].len;
            prev_processed = 1; 
        }

        addToPeriodList(p_list, &prd);       

        for (i = prev_processed; i < prd_array[(unsigned char)data[index]].count - 1; i++)
        {
            addToPeriodList(p_list, &prd_array[(unsigned char)data[index]].prd_table[i]);
            printf("third val: %ld\n", prd.len);
        }

        prev_len = 0;

        if (prd_array[(unsigned char)data[index]].count > 1)
        {
            if ((index < data_size - 1) && 
                (prd_array[(unsigned char)data[index]].prd_table[i].val == prd_array[(unsigned char)data[index + 1]].prd_table[0].val)) {
                prev_len = prd_array[(unsigned char)data[index]].prd_table[i].len;
            } else { 
                prd.len = prd_array[(unsigned char)data[index]].prd_table[i].len;
                prd.val = prd_array[(unsigned char)data[index]].prd_table[i].val;
                prd.count = 1;
                addToPeriodList(p_list, &prd);
            }
        }
        index++;
    }
}
