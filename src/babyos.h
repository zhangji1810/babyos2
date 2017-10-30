/*
 * guzhoudiaoke@126.com
 * 2017-10-26
 */

#ifndef _BABYOS_H_
#define _BABYOS_H_

#include "screen.h"
#include "console.h"
#include "mm.h"
#include "arch.h"
#include "ide.h"

class babyos_t {
public:
    babyos_t();
    ~babyos_t();

    void run();

    screen_t*   get_screen();
    console_t*  get_console();
    mm_t*       get_mm();
    arch_t*     get_arch();
    ide_t*      get_ide();

    static babyos_t* get_os();

private:
    screen_t	m_screen;
    console_t   m_console;
    mm_t		m_mm;
    arch_t      m_arch;
    ide_t       m_ide;
};

#define os()	    babyos_t::get_os()
#define console()   os()->get_console()

#endif