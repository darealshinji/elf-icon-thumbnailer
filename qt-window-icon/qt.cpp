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


class myelfpng
{
private:
    const char *m_filename;
    void *m_addr = MAP_FAILED;
    size_t m_filesize = 0;
    size_t m_num = 0;
    struct elfpng_section *m_sec = NULL;

public:

    myelfpng(const char *filename) : m_filename(filename)
    {}

    ~myelfpng()
    {
        if (m_addr != MAP_FAILED) {
            munmap(m_addr, m_filesize);
        }

        free(m_sec);
    }

    bool load_sections()
    {
        if (m_addr == MAP_FAILED) {
            if ((m_addr = elfpng_open_file(m_filename, &m_filesize)) == MAP_FAILED ||
                (m_sec = elfpng_data(m_addr, m_filesize, &m_num)) == NULL)
            {
                return false;
            }
        }

        return true;
    }

    struct elfpng_section *sections() const {
        return m_sec;
    }

    size_t size() const {
        return m_num;
    }
};


static void set_application_icon(QApplication *app)
{
    myelfpng png("/proc/self/exe");

    if (!png.load_sections()) {
        return;
    }

    auto sec = png.sections();
    size_t n = 0;

    /* get largest icon */
    for (size_t i = 1; i < png.size(); i++) {
        if (sec[i].height > sec[n].height) {
            n = i;
        }
    }

    QPixmap pixmap;

    if (pixmap.loadFromData(sec[n].data, sec[n].datasize, "PNG")) {
        app->setWindowIcon(QIcon(pixmap));

        std::cout << "icon size is " << sec[n].width << " x " << sec[n].height
            << std::endl;
    }
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
