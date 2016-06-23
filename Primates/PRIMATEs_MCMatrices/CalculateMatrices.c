#include <stdio.h>

int T2(int value);
int T4(int value);
int T9(int value);
int T15(int value);
void MultiMatrices(int newMatrix[7][7], int old[7][7]);
void MultiInvMatrices(int newMatrix[7][7], int old[7][7]);
void printMatrix(int m[7][7]);
void printForwardMatrices();
void printBackwardsMatrices();

void main() {

	printBackwardsMatrices();

	getchar();
}

void printBackwardsMatrices() {
	//INV_A1
	int INV_A1[7][7] = {
	2, 15, 9, 9, 15, 2, 1,
	1, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 1, 0 };

	//A2
	int INV_A2[7][7] = { 0 };

	//A3
	int INV_A3[7][7] = { 0 };

	//A4
	int INV_A4[7][7] = { 0 };

	//A5
	int INV_A5[7][7] = { 0 };

	//A6
	int INV_A6[7][7] = { 0 };

	//A7
	int INV_A7[7][7] = { 0 };

	MultiInvMatrices(INV_A2, INV_A1);
	MultiInvMatrices(INV_A3, INV_A2);
	MultiInvMatrices(INV_A4, INV_A3);
	MultiInvMatrices(INV_A5, INV_A4);
	MultiInvMatrices(INV_A6, INV_A5);
	MultiInvMatrices(INV_A7, INV_A6);

	printf("Inv_A1: \n");
	printMatrix(INV_A1);

	printf("Inv_A2: \n");
	printMatrix(INV_A2);

	printf("Inv_A3: \n");
	printMatrix(INV_A3);

	printf("Inv_A4: \n");
	printMatrix(INV_A4);

	printf("Inv_A5: \n");
	printMatrix(INV_A5);

	printf("Inv_A6: \n");
	printMatrix(INV_A6);

	printf("Inv_A7: \n");
	printMatrix(INV_A7);
}


void MultiInvMatrices(int newMatrix[7][7], int old[7][7]) {

	//Iterate over all rows but the first, and move the values down one row.
	for (int row = 1; row < 7; row++) {
		for (int col = 0; col < 7; col++) {
			newMatrix[row][col] = old[row - 1][col];
		}
	}

	//first row [2 15 9 9 15 2 1]
	for (int col = 0; col < 7; col++) {
		newMatrix[0][col] = T2(old[0][col]) ^ T15(old[1][col]) ^ T9(old[2][col]) ^ T9(old[3][col]) ^ T15(old[4][col]) ^ T2(old[5][col]) ^ old[6][col];
	}
}

void printForwardMatrices() {
	//A1
	int A1[7][7] =
	{ 0, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 1,
		1, 2, 15, 9, 9, 15, 2 };

	//A2
	int A2[7][7] = { 0 };

	//A3
	int A3[7][7] = { 0 };

	//A4
	int A4[7][7] = { 0 };

	//A5
	int A5[7][7] = { 0 };

	//A6
	int A6[7][7] = { 0 };

	//A7
	int A7[7][7] = { 0 };

	MultiMatrices(A2, A1);
	MultiMatrices(A3, A2);
	MultiMatrices(A4, A3);
	MultiMatrices(A5, A4);
	MultiMatrices(A6, A5);
	MultiMatrices(A7, A6);

	printf("A1: \n");
	printMatrix(A1);

	printf("A2: \n");
	printMatrix(A2);

	printf("A3: \n");
	printMatrix(A3);

	printf("A4: \n");
	printMatrix(A4);

	printf("A5: \n");
	printMatrix(A5);

	printf("A6: \n");
	printMatrix(A6);

	printf("A7: \n");
	printMatrix(A7);
}

void printMatrix(int m[7][7]) {
	for (int row = 0; row < 7; row++) {
		for (int col = 0; col < 7; col++) {
			printf("%i\t", m[row][col]);
		}
		printf("\n");
	}
	printf("\n");
}

void MultiMatrices(int newMatrix[7][7], int old[7][7]) {

	//Iterate over all rows but the last, and move the values up one row.
	for (int row = 0; row < 6; row++) {
		for (int col = 0; col < 7; col++) {
			newMatrix[row][col] = old[row + 1][col];
		}
	}

	//Last row [1 2 15 9 9 15 2]
	for (int col = 0; col < 7; col++) {
		newMatrix[6][col] = old[0][col] ^ T2(old[1][col]) ^ T15(old[2][col]) ^ T9(old[3][col]) ^ T9(old[4][col]) ^ T15(old[5][col]) ^ T2(old[6][col]);
	}
}




int T2(int value) {
	
	//Shift each bit in the int.
	int newValue = value << 1;

	//Reduce with polynomial 100101 if needed
	int reduceBit = newValue >> 5;

	newValue ^= reduceBit;
	newValue ^= (reduceBit << 5);
	newValue ^= (reduceBit << 2);

	return newValue;
}

int T8(int value) {

	return T2(T2(T2(value)));
}

int T4(int value) {

	return T2(T2(value));
}

int T9(int value) {
	return T8(value) ^ value;
}

int T15(int value) {
	return T8(value) ^ T4(value) ^ T2(value) ^ value;
}