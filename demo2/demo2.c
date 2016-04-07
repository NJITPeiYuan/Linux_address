#include <stdio.h>
#include <string.h>

#define MAX_SIZE 1024

void reverse_string(char *src, int len)
{
    char temp;
    int i;

    for(i = 0; i < len/2; i++)
    {
        temp = *(src + i);
	*(src + i) = *(src + len - 1 - i);
	*(src + len - 1 - i) = temp;
    }
}

void overturn(char *str)
{
    int len = 0;

    reverse_string(str, strlen(str));        //整体倒序

    while(*str != '\0')
    {
        if(*str == ' ')
	{
	    reverse_string(str - len, len);   //遇到空格时倒序

	    str++;
	    len = 0;
	}
	str++;
	len++;
    }

    reverse_string(str - len, len);          //最后的'\0' 倒序
}

int main()
{
    char str[MAX_SIZE] = "i am from shanghai";

    printf("Please input the string: %s \n", str);

    overturn(str);

    printf("the finally string is : %s\n", str);

    return 0;
}
