#include "ppm_image.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

static const string_view PPM_SIG = "P6"sv;
static const int PPM_MAX = 255;

// реализуйте эту функцию самостоятельно
bool SavePPM(const Path& file, const Image& image) {
    bool res = false;
    ofstream ofs(file, ios::binary);
    
    // Узнаем параметры изображения:
    int w, h, step;
    try {
        w = image.GetWidth();
        h = image.GetHeight();
        step = image.GetStep();
    } catch (...) {
        return res;
    }
    
    // Записываем заголовок
    ofs << PPM_SIG << "\n" << w << " " << h << "\n"  << PPM_MAX << "\n";
    
    std::vector<char> buff(w * 3);
    try{
        // Идем по строкам изображения 
        for (int y = 0; y < h; ++y) {
            // берем очередную строку изображения
            const Color* color_line = image.GetLine(y);
            for (int x = 0; x < w; ++x) {
                buff[3 * x] = static_cast<char>(color_line[x].r);
                buff[3 * x + 1] = static_cast<char>(color_line[x].g);
                buff[3 * x + 2] = static_cast<char>(color_line[x].b);
            }
            // Заполняем padding
            for (int x = w; x < step; x++) {
                buff[3 * x] = static_cast<char>(Color::Black().r);
                buff[3 * x + 1] = static_cast<char>(Color::Black().g);
                buff[3 * x + 2] = static_cast<char>(Color::Black().b);
            }
            ofs.write(buff.data(), w * 3);
        }
    } catch (...) {
        return res;
    }
    res = ofs.good();

    return res;

}

Image LoadPPM(const Path& file) {
    // открываем поток с флагом ios::binary
    // поскольку будем читать даные в двоичном формате
    ifstream ifs(file, ios::binary);
    std::string sign;
    int w, h, color_max;

    // читаем заголовок: он содержит формат, размеры изображения
    // и максимальное значение цвета
    ifs >> sign >> w >> h >> color_max;

    // мы поддерживаем изображения только формата P6
    // с максимальным значением цвета 255
    if (sign != PPM_SIG || color_max != PPM_MAX) {
        return {};
    }

    // пропускаем один байт - это конец строки
    const char next = ifs.get();
    if (next != '\n') {
        return {};
    }

    Image result(w, h, Color::Black());
    std::vector<char> buff(w * 3);

    for (int y = 0; y < h; ++y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), w * 3);

        for (int x = 0; x < w; ++x) {
            line[x].r = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].b = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
}

}  // namespace img_lib