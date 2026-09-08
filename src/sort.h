#ifndef SORT_H
#define SORT_H

//Using the V_ prefix to mean "Various"
//Other prior prefixes didn't fit (like P_(hysics) or G_(eometry), etc.)


#define v_size_t unsigned long int


static void v_swap(void* a, void* b, v_size_t size) {
	//Swaps bytes in a given range.
	unsigned char* x = (unsigned char*)(a);
	unsigned char* y = (unsigned char*)(b);

	//Iterate through.
	while (size--) {
		unsigned char tmp = *x;
		*(x++) = *y;
		*(y++) = tmp;
	}
}


static void v_quickSort(
	unsigned char* array,
	v_size_t lo, v_size_t hi,
	v_size_t elementSize,
	int (*compare)(const void*, const void*)
) {
	if ((hi - lo) < 2u) {return;}

	v_size_t i = lo;
	v_size_t j = hi - 1u;
	v_size_t pivotIndex = lo + (hi - lo) / 2u;
	unsigned char pivot[elementSize];

	for (v_size_t k=0u; k<elementSize; k++) {
		pivot[k] = array[pivotIndex * elementSize + k];
	}

	while (i <= j) {

		while (compare(
			array + i * elementSize, pivot
		) < 0) {
			i++;
		}

		while (compare(
			array + j * elementSize,
			pivot
		) > 0) {
			if (j == lo) {break;}
			j--;
		}

		if (i <= j) {
			v_swap(
				array + i * elementSize,
				array + j * elementSize,
				elementSize
			);
			i++;

			if (j > lo) {j--;}
			else {break;}
		}
	}

	if (lo < j) {
		v_quickSort(
			array, lo, j + 1u,
			elementSize, compare
		);
	}

	if (i < hi) {
		v_quickSort(
			array, i, hi,
			elementSize, compare
		);
	}
}


static void v_sort(
	void* array,
	v_size_t arrayCount,
	v_size_t elementSize,
	int (*compare)(const void*, const void*)
)
{
	if (
		(!array) || (!compare) ||
		(arrayCount < 2) || (elementSize == 0)
	) {return; /* Early return if missing info or unsortable (Too short, no compare given) */}

	v_quickSort(
		(unsigned char*)array,
		0, arrayCount, elementSize,
		compare
	);
}


#endif
