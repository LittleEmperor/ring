
#include "libringbuf.h"

int main(void)
{
	unsigned int i = 0;
	void *obj;
	struct ringbuf *r = libringbuf_create((1<<6), 0);
	if(NULL == r){
		printf("create ringbuf err\n");
		return -1;
	}
	while(!libringbuf_full(r)){
		i++;
		libringbuf_sp_enqueue(r, *(void **)&i);
	}

	while(!libringbuf_empty(r)){
		i++;
		libringbuf_sc_dequeue(r, &obj);
		printf("%d\n", *(unsigned int *)&obj);
	}

	printf("all dequeue ok\n");

	libringbuf_free(r);
	return 0;
}
