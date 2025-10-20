/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Carsten Janssen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QWidget>
#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include "../elfpng.h"


static void set_application_icon(QApplication *app)
{
    size_t filesize, num;
    void *addr = elfpng_open_file("/proc/self/exe", &filesize);

    if (addr == MAP_FAILED) {
        return;
    }

    auto sec = elfpng_data(addr, filesize, &num);

    if (sec) {
        QPixmap pixmap;
        size_t n = 0;

        /* get largest icon */
        for (size_t i = 1; i < num; i++) {
            if (sec[i].h > sec[n].h) {
                n = i;
            }
        }

        if (pixmap.loadFromData(sec[n].data, sec[n].size, "PNG")) {
            app->setWindowIcon(QIcon(pixmap));
            std::cout << "icon size is " << sec[n].w << " x " << sec[n].h << std::endl;
        }

        free(sec);
    }

    munmap(addr, filesize);
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    set_application_icon(&app);

    QWidget window;
    window.resize(250, 150);
    window.setWindowTitle("Test");
    window.show();

    return app.exec();
}
