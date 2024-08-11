#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <iostream>

using namespace std;

namespace img_lib {

static const char BMP_SIGN[2] = {'B', 'M'}; 
static const int BMP_INFO_HEADER_SIZE = 40;
static const int BMP_FILE_HEADER_SIZE = 14;


// поля заголовка Bitmap File Header
PACKED_STRUCT_BEGIN BitmapFileHeader {
    char sign[2] = {'B', 'M'};
    uint32_t full_size;  // Суммарный размер заголовка (54 байта) и данных 
    // (Размер данных определяется как отступ, умноженный на высоту изображения)
    uint32_t reserved_space = 0;  // Зарезервированное пространство — 4 байта, заполненные нулями.
    uint32_t data_shift = BMP_INFO_HEADER_SIZE + BMP_FILE_HEADER_SIZE;  // Отступ данных от начала файла. Он равен размеру двух частей заголовка.
}
PACKED_STRUCT_END

// поля заголовка Bitmap Info Header
PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t info_header_size = BMP_INFO_HEADER_SIZE;  //Размер заголовка. Учитывается только размер второй части заголовка.
    int32_t img_width;  // Ширина изображения в пикселях — 4 байта, знаковое целое.
    int32_t img_height;  // Высота изображения в пикселях — 4 байта, знаковое целое.
    uint16_t num_of_layers = 1;  // Количество плоскостей. В нашем случае всегда 1 — одна RGB плоскость.
    uint16_t bits_per_pixel = 24;  // Количество бит на пиксель. В нашем случае всегда 24.
    uint32_t compress_type = 0;  // Тип сжатия. В нашем случае всегда 0 — отсутствие сжатия.
    uint32_t data_size;  // Количество байт в данных. Произведение отступа на высоту.
    int32_t h_resolution = 11811;  // Горизонтальное разрешение, пикселей на метр. 11811 соответствует 300 DPI.
    int32_t v_resolution = 11811;  // Вертикальное разрешение, пикселей на метр. 11811 соответствует 300 DPI.
    int32_t num_of_used_colors = 0;  // Количество использованных цветов. 0 — значение не определено.
    int32_t num_of_valuable_colors = 0x1000000;
}
PACKED_STRUCT_END


// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}


// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image){
    bool res = false;
    ofstream ofs(file, ios::binary);

    if (!ofs.is_open()) {
        std::cerr << "Error in input file opening"sv << std::endl;
        return res;
    }

    
    // Инициируем структуры header-ов BMP
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    // вычисляем отступ
    int stride = GetBMPStride(image.GetWidth());

    info_header.img_height = image.GetHeight();
    info_header.img_width = image.GetWidth();
    info_header.data_size = stride * info_header.img_height;  // размер данных в байтах = произведение отступа на высоту
    info_header.info_header_size = sizeof(info_header);

    file_header.full_size = sizeof(file_header) + sizeof(info_header) + info_header.data_size;
    
    // Записываем заголовки
    ofs.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ofs.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    if (!ofs.good()) {
        std::cerr << "Error in writing the headers"sv << std::endl;
        return res;
    }
    
    // В буфер будем записывать строку изображения с учетом padding
    std::vector<char> buff(stride);
    try{
        // Идем по строкам изображения 
        for (int y = (info_header.img_height - 1) ; y >= 0; --y) {
            // берем очередную строку изображения
            const Color* color_line = image.GetLine(y);
            for (int x = 0; x < info_header.img_width; ++x) {
                buff[3 * x] = static_cast<char>(color_line[x].b);
                buff[3 * x + 1] = static_cast<char>(color_line[x].g);
                buff[3 * x + 2] = static_cast<char>(color_line[x].r);
            }
            // Заполняем padding
            for (int i = 3 * info_header.img_width; i < stride; i++) {
                // цвета в BMP в обратном порядке blue-green-red
                buff[i] = static_cast<char>(0);
            }
            ofs.write(buff.data(), stride);
        }
    } catch (...) {
        std::cerr << "Error in image writing"sv << endl;
        return res;
    }
    res = ofs.good();

    return res;
}


// напишите эту функцию
Image LoadBMP(const Path& file) {
    // открываем поток с флагом ios::binary
    // поскольку будем читать даные в двоичном формате
    ifstream ifs(file, ios::binary);

    if (!ifs.is_open()) {
        std::cerr << "Error in output file opening"sv << std::endl;
        return {};
    }

    std::string sign;
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    // читаем заголовки
    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    if (!ifs.good()) {
        std::cerr << "Error in reading the headers"sv << std::endl;
        return {};
    }


    // мы поддерживаем изображения только формата P6
    // с максимальным значением цвета 255
    if (file_header.sign[0] != BMP_SIGN[0] || file_header.sign[1] != BMP_SIGN[1]) {
        std::cerr << "Incorrect signature of file" << std::endl;
        return {};
    }


    // определяем отступ
    int stride = GetBMPStride(info_header.img_width);

    if (stride * info_header.img_height != info_header.data_size) {
        std::cerr << "Incorrect stride or data size in BMP info header"sv << std::endl;
        return {};
    }

    Image result_img(info_header.img_width, info_header.img_height, Color::Black());
    // буфер размером в отступ (3 ширины и добавка до целого количества блоков по 4 байта)
    std::vector<char> buff(stride);

    for (int y = info_header.img_height - 1; y >= 0; --y) {
        Color* line = result_img.GetLine(y);
        // читаем всю строку (c padding-ом)
        ifs.read(buff.data(), stride);
        
        // а записываем в изображение без padding-а
        for (int x = 0; x < info_header.img_width; ++x) {
            // цвета в обратном порядке
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }    
    }

    if (!ifs.good()) {
        std::cerr << "Error in image writing"sv << std::endl;
        return{};
    }

    return result_img;
}


}  // namespace img_lib