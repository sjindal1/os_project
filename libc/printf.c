
// printf for user part

#include <sys/stdarg.h>
#include <unistd.h>
#include <string.h>

char *getStrFromInt(char *str, int arg_val, int code_type);
char *getStrFromUnsignlong(char *str, unsigned long arg_val, int code_type);
char *strconcat(char *dest, char* str1, char* str2);
void displayBuff();


char final_str[1000];
char str[1000];


void displayBuff(){
	write(1, final_str, strlen(final_str));
}


void printf(const char *fmt, ...){
    va_list valist;
    va_start(valist,fmt);
    
    int i=0;
    for(;*fmt!='\0';fmt++){
        if(*fmt == '\\'){
            fmt++;
        }else{
            if(*fmt == '%'){
                char *str1;
                char str_pointer[25];
                str1 = (char*) &str[0];
                str[0] = '\0';
                switch(*(++fmt)){
                    case 's' :
                        str1 = va_arg(valist,char *);
                        break;
                    case 'c' :
                        final_str[i++] = va_arg(valist,int);
                        break;
                    case 'd' :
                        str1 = getStrFromInt(str, va_arg(valist,int), 10);
                        break;
                    case 'x' :
                        str1 = getStrFromUnsignlong(str, va_arg(valist,unsigned long), 16);
                        break;
                    case 'p' :
                        str1 = strconcat(str,"0x",getStrFromUnsignlong(str_pointer, va_arg(valist,unsigned long), 16));
                        break;
                    default :
                        ;
                }
                for(;*str1!='\0';str1++)
                    final_str[i++] = *str1;
                str1 = (char*) &str[0];
            }
            else{
                final_str[i++] = *fmt;
            }
        }
    }
    final_str[i] = '\0';
    va_end(valist);
    
    displayBuff();
    
 	return;
}

char *strconcat(char *dest, char *str1,  char *str2){
    int i=0;
    for(;str1[i]!='\0';i++){
        dest[i] = str1[i];
    }
    for(int j=0;str2[j]!='\0';j++,i++){
        dest[i] = str2[j];
    }
    dest[i] = '\0';
    return dest;
}

char *getStrFromInt(char *str, int arg_val, int code_type){
    int i=0, neg = 0, j, k;
    if(arg_val < 0)
    {
        arg_val = ~arg_val + 1;
        neg = 1;
        str[i++] = '-';
    }
    int rem=arg_val%code_type;

    if(arg_val == 0)		// handle special condition
    {
      str[i++] = '0';
      str[i] = '\0';
      return str;
    }

    while(arg_val != 0){
        if(rem < 10)
            str[i] = rem+48;
        else
            str[i] = rem+55;
        i++;
        arg_val/=code_type;
        rem = arg_val%code_type;
    }
    if(neg)
    {
        for(j = 1, k = 0; j < (i+1)/2;j++, k++)
        {
            char temp = str[j];
            str[j] = str[i-1-k];
            str[i-1-k] = temp;
        }
    }
    else
    {
        for(j=0;j<i/2;j++){
            char temp = str[j];
            str[j] = str[i-1-j];
            str[i-1-j] = temp;
        }
    }
    
    str[i] = '\0';
    return str;
}


char *getStrFromUnsignlong(char *str, unsigned long arg_val, int code_type) {
    int i = 0, j;
    char t;
    int rem = arg_val % code_type;
        
    if(arg_val == 0)            // handle special condition
    {
      str[i++] = '0';
      str[i] = '\0';
      return str;
    }

    while(arg_val != 0)
    {
        if(rem < 10)
            str[i] = rem + 48;
        else
            str[i] = rem + 55;
        i++;
        arg_val /= code_type;
        rem = arg_val % code_type;
    }
    
    for(j = 0; j < i/2; j++)
    {
        t = str[j];
        str[j] = str[i - 1 - j];
        str[i - 1 - j] = t;
    }
    
    str[i] = '\0';
    return str;
}


