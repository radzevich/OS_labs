#include <stdio.h>
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
	struct period prd_table[BLOCK_SIZE / 2];
	int count;
};

typedef struct period_set prd_set;


void processValue(prd_set *p_set, char val);
void setValue(prd_set *p_set, char prd_val, long prd_len);



int main(int argc, char const *argv[])
{
	prd_set prd_array[2 << BLOCK_SIZE];
	FILE *file;

	memset((char*)prd_array, 0, sizeof(prd_array));
	
	for (int i = 0; i < 1 << BLOCK_SIZE; i++)
	{
		processValue(&(prd_array[i]), i);
	}

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
			setValue(p_set, prev_bit_val, prd_len);
			prd_len = 1;
		}

		if (0 == shift)
		{
			setValue(p_set, cur_bit_val, prd_len);
		}

		prev_bit_val = cur_bit_val;
		shift--;
	}
}


void setValue(prd_set *p_set, char prd_val, long prd_len)
{
	for (int i = 0; i < p_set->count; i++)
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
