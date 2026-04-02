#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <iostream>

// Ця функція викликається щоразу, коли мікрофон отримує порцію звуку
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    // Тут ми будемо аналізувати звук
    (void)pOutput; // Не використовуємо динаміки
    
    std::cout << "Отримано порцію звуку: " << frameCount << " фреймів" << std::endl;
}

int main() {
    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;

    // Налаштовуємо пристрій на запис (capture)
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format   = ma_format_f32; // Працюємо з числами з комою (0.0 - 1.0)
    deviceConfig.capture.channels = 1;             // Моно-звук
    deviceConfig.sampleRate       = 44100;         // Стандартна частота
    deviceConfig.dataCallback     = data_callback; // Вказуємо нашу функцію обробки

    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        std::cerr << "Помилка ініціалізації мікрофона!" << std::endl;
        return -1;
    }

    ma_device_start(&device);

    std::cout << "Натисніть Enter, щоб вийти..." << std::endl;
    std::cin.get();

    ma_device_uninit(&device);
    return 0;
}