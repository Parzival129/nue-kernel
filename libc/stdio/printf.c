#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;

		} else if (*format == 'd') {
			format++;
			int n = va_arg(parameters, int);
			unsigned int magnitude;
			if (n < 0)
				magnitude = (unsigned int) (-(n + 1)) + 1;
			else
				magnitude = (unsigned int) n;
			char buf[12];
			int i = 0;
			if (magnitude == 0) {
				buf[i++] = '0';
			} else {
				char tmp[12];
				int j = 0;
				while (magnitude > 0) {
					tmp[j++] = '0' + (magnitude % 10);
					magnitude /= 10;
				}
				while (j > 0) {
					buf[i++] = tmp[--j];
				}
			}
			size_t len = (size_t) i + (n < 0 ? 1u : 0u);
			if (maxrem < len) {
				return -1;
			}
			if (n < 0 && !print("-", 1))
				return -1;
			if (!print(buf, i))
				return -1;
			written += len;


		// include functionality for hexademical, specifically for IRQ handler logging
		// for *format u and x
		} else if (*format == 'u') {
			format++;
			unsigned int n = va_arg(parameters, unsigned int);
			char buf[12];
			int i = 0;
			if (n == 0) {
				buf[i++] = '0';
			} else {
				char tmp[12];
				int j = 0;
				while (n > 0) {
					tmp[j++] = '0' + (n % 10);
					n /= 10;
				}
				while (j > 0) {
					buf[i++] = tmp[--j];
				}
			}
			if (maxrem < (size_t)i) {
				return -1;
			}
			if (!print(buf, i))
				return -1;
			written += i;

		} else if (*format == 'x') {
			format++;
			unsigned int n = va_arg(parameters, unsigned int);
			char buf[9];
			int i = 0;
			if (n == 0) {
				buf[i++] = '0';
			} else {
				char tmp[9];
				int j = 0;
				while (n > 0) {
					int digit = n % 16;
					tmp[j++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
					n /= 16;
				}
				while (j > 0) {
					buf[i++] = tmp[--j];
				}
			}
			if (maxrem < (size_t)i) {
				return -1;
			}
			if (!print(buf, i))
				return -1;
			written += i;

		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
