// Программа для конвертации JPEG в PPM

#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>
#include <bmp_image.h>

#include <filesystem>
#include <string_view>
#include <iostream>

using namespace std;


namespace format_interface {


enum class Format{
    PPM,
    JPEG,
    BMP,
    UNKNOWN
};


Format GetFormatByExtension(const img_lib::Path& input_file) {
    const string ext = input_file.extension().string();
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }

    if (ext == ".ppm"sv) {
        return Format::PPM;
    }

    if (ext == ".bmp"sv) {
        return Format::BMP;
    }

    return Format::UNKNOWN;
}


class ImageFormatInterface {
public:
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
}; 


class PpmFormatInterface : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }

    virtual img_lib::Image LoadImage(const img_lib::Path& file) const override{
        return img_lib::LoadPPM(file);
    }
};


class JpegFormatInterface : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }

    virtual img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};

class BmpFormatInterface : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveBMP(file, image);
    }

    virtual img_lib::Image LoadImage(const img_lib::Path& file) const override{
        return img_lib::LoadBMP(file);
    }
};


// Возвращает указатель на интерфейс нужного формата или nullptr, 
// если формат не удалось определить
ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    Format fmt = GetFormatByExtension(path);
    
    switch(fmt) {
        case Format::PPM:
            // статическая переменная будет жить на протяжении всей жизни программы 
            // static const PpmFormatInterface ppm_interface;
            return new PpmFormatInterface;
            break;
        case Format::JPEG:
            // static const JpegFormatInterface jpeg_interface;
            return new JpegFormatInterface;
            break;
        case Format::BMP:
            return new BmpFormatInterface;
            break;
        default:
            return nullptr;
    }

    return nullptr;
} 


}  // namespace format_interface


int main(int argc, const char** argv) {
    
    // 0. Проверить количество аргументов
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];

    // 1. Проверить формат входного файла
    format_interface::ImageFormatInterface* fmt_interface_in = format_interface::GetFormatInterface(in_path);
    if (!fmt_interface_in) {
        cerr << "Unknown format of the input file."sv << endl;
        return 2;
    }

    // 2. Проверить формат выходного файла
    format_interface::ImageFormatInterface* fmt_interface_out = format_interface::GetFormatInterface(out_path);
    if (!fmt_interface_out) {
        cerr << "Unknown format of the output file."sv << endl;
        return 3;
    }

    img_lib::Image image = fmt_interface_in->LoadImage(in_path);
    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }

    if (!fmt_interface_out->SaveImage(out_path, image)) {
        cerr << "Saving failed"sv << endl;
        return 5;
    }

    cout << "Successfully converted"sv << endl;
    return 0;

}