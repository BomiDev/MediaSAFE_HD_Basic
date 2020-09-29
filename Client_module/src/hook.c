#define _GNU_SOURCE
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdio.h>
#include <dlfcn.h>

/*
gcc -shared -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ -fPIC -ldl -o hook.so hook.c

*/
// ���� printf�� �����ϴ� �޸� �ּҸ� �����ϰ� �ִ�.
guchar * (*gdk_pixbuf_get_pixels_real)(const GdkPixbuf *) = NULL;

// ���̺귯���� �ε�� �� ����ȴ�.
void __attribute__((constructor)) init_hooking()
{
    // ��¥ printf�� �����ϴ� �޸� �ּҸ� �����´�.
    gdk_pixbuf_get_pixels_real = dlsym(RTLD_NEXT, "gdk_pixbuf_get_pixels");
    fprintf (stderr, "===real printf is at %p\n", gdk_pixbuf_get_pixels);
}

guchar *gdk_pixbuf_get_pixels (const GdkPixbuf *pixbuf){
	fprintf (stderr, "=================== gdk_pixbuf_get_pixels_real ==========================\r\n");
	printf("=================== gdk_pixbuf_get_pixels_real ==========================\r\n");
 
 return NULL;
 return gdk_pixbuf_get_pixels_real(pixbuf);



}
/*
void *memcpy(void *dest, const void *src, size_t count) {
        char* dst8 = (char*)dest;
        char* src8 = (char*)src;
 
        while (count--) {
            *dst8++ = *src8++;
        }
 printf("H4ck : memcpy[%x][%s]\r\n",dest,src);
        return dest;
}
*/