#include "jpeg_image.h"

#include <jpeglib.h>


#include <csetjmp>
#include <cstddef>


namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
/*
 * Here's the routine that will replace the standard error_exit method:
 */
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}


// тип JSAMPLE фактически псевдоним для unsigned char
void SaveScanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{std::byte{pixel[0]}, std::byte{pixel[1]}, std::byte{pixel[2]}, std::byte{255}};
    }
}


img_lib::Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }

    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    /* Шаг 2: устанавливаем источник данных */

    jpeg_stdio_src(&cinfo, infile);

    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);

    /* Шаг 4: устанавливаем параметры декодирования */

    // установим желаемый формат изображения
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    /* Шаг 5: начинаем декодирование */

    (void) jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    /* Шаг 6: while (остаются строки изображения) */
    /*                     jpeg_read_scanlines(...); */

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveScanlineToImage(buffer[0], y, result);
    }

    /* Шаг 7: Останавливаем декодирование */

    (void) jpeg_finish_decompress(&cinfo);

    /* Шаг 8: Освобождаем объект декодирования */

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}




// тип JSAMPLE фактически псевдоним для unsigned char
void SaveImageLineToJPEGRow(const Image& in_image, int y, JSAMPLE* row) {
    const Color* line = in_image.GetLine(y);
    for (int x = 0; x < in_image.GetWidth(); ++x) {
        JSAMPLE* pixel = row + x * 3;
        // line[x] = Color{std::byte{pixel[0]}, std::byte{pixel[1]}, std::byte{pixel[2]}, std::byte{255}};
        pixel[0] = static_cast<JSAMPLE>(line[x].r);
        pixel[1] = static_cast<JSAMPLE>(line[x].g);
        pixel[2] = static_cast<JSAMPLE>(line[x].b);
        // pixel[3] = static_cast<JSAMPLE>(line[x].a);
    }
}

// В эту функцию вставлен код примера из библиотеки libjpeg.
// Измените его, чтобы адаптировать к переменным file и image.
// Задание качества уберите - будет использовано качество по умолчанию
bool SaveJPEG(const Path& file, const Image& image) {
    
    // Структура для хранения параметров изображения и сжатия из примера    
    jpeg_compress_struct cinfo;
    
    // Структура из примера для хранения и вывода ошибок    
    jpeg_error_mgr jerr;
    /* More stuff */
    FILE * outfile;       /* target file */
    JSAMPROW row_pointer[1];  /* pointer to JSAMPLE row[s] */
    int row_stride;       /* physical row width in image buffer */

    /* Step 1: allocate and initialize JPEG compression object */
    /* Шаг 1. Инициализация объекта JPEG */
    
    // перед инициализацией запишем в данные указатель на менеджер ошибок, а то вдруг при инициализации ошибка будет
    cinfo.err = jpeg_std_error(&jerr);  
    
    jpeg_create_compress(&cinfo);
    
    // Шаг 2. Устанавливаем файл, куда будем записывать изображение

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((outfile = _wfopen(file.wstring().c_str(), "wb")) == NULL) {
#else
    if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
#endif
        // Исключили диагностические сообщения, выводимые функцией fprintf. 
        return false;  // При ошибке вместо вызова exit верните false.
    }

    jpeg_stdio_dest(&cinfo, outfile);

    // Шаг 3. Устанавливаем параметры изображения

    cinfo.image_width = image.GetWidth();  /* image width and height, in pixels */
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = 3;       /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB;   /* colorspace of input image */
    
    // Устанавливаем параметры по умолчанию
    jpeg_set_defaults(&cinfo);
    
    /* Далее можно было бы установить специальные параметры, в частности, качество изображения
    // Убрали установку качества изображения, будет использоваться качество по умолчанию

    /* Шаг 4. Запуск сжатия */

    /* TRUE ensures that we will write a complete interchange-JPEG file.
    * Pass TRUE unless you are very sure of what you're doing.
    */
    jpeg_start_compress(&cinfo, TRUE);

    /* Шаг 5: построчная запись while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */

    
    row_stride = cinfo.image_width * 3; /* JSAMPLEs per row in image_buffer */

    // Построчный цикл
    // статическая переменная cinfo.next_scanline используется как счетчик
    while (cinfo.next_scanline < cinfo.image_height) {
    // из изображения записываем строку в буфер попиксельно, 
    // а из буфера затем разом в jpeg-объект 
    int y = cinfo.next_scanline;
    std::vector<JSAMPLE> image_buffer(row_stride);
    SaveImageLineToJPEGRow(image, y, &image_buffer[0]); 

    row_pointer[0] = & image_buffer[0];  /* было [cinfo.next_scanline * row_stride */;
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Шаг 6: Завершение записи/сжатия */

    jpeg_finish_compress(&cinfo);
    /* After finish_compress, we can close the output file. */
    fclose(outfile);

    /* Шаг 7: освобождаем память (release JPEG compression object) */
    jpeg_destroy_compress(&cinfo);

    /* And we're done! */
    // В случае успеха возвратите true
    return true; 
}


}  // namespace img_lib