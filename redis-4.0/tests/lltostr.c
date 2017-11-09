#include <stdio.h>

int sdsll2str(char *s, long long value) {
	char *p, aux;
	unsigned long long v;
	size_t l;

	/* Generate the string representation, this method produces
	 *      * an reversed string. */
	/* 通过取余数得到原字符串的逆转形式 */
	v = (value < 0) ? -value : value;
	p = s;
	do {
		*p++ = '0'+(v%10);
		v /= 10;
	} while(v);
	if (value < 0) *p++ = '-';

	/* Compute length and add null term. */
	l = p-s;
	*p = '\0';

	/* Reverse the string. */
	/* 反转字符串 */
	p--;
	while(s < p) {
		aux = *s;
		*s = *p;
		*p = aux;
		s++;
		p--;
	}
	return l;
}

int main()
{
	int num = 1234567890;
	char str[21];

	// 整数
	int len = sdsll2str(str, num);
	printf("len: %d string %s\n", len, str);

	// 负数
	num = -1234567890;
	len = sdsll2str(str, num);
	printf("len: %d string %s\n", len, str);

	// 超出21，会溢出
	num = 1234567890123456789011;
	len = sdsll2str(str, num);
	printf("len: %d string %s\n", len, str);

	return 0;
}