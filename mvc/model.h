#ifndef _MODEL_H_
#define _MODEL_H_

#define MODEL_DATA_SIZE (16 * 16)

typedef struct dot_model {
	unsigned char data[MODEL_DATA_SIZE];
	// int size;

	int (*set)(int x, int y, int color);
	int (*get)(int x, int y);

	// int (bulk_set)();
} dot_model_t;

dot_model_t dot_model;

#endif /* _MODEL_H_ */