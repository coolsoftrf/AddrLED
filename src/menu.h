#pragma once
#include <Stream.h>
#include "types.h"

//#define DUMP_STACK_CHANGES

const char KEY_AUTO = '\0';
const char KEY_RESET_MENU = '\1';
const char KEY_POP_MENU = '\2';
const char KEY_INT_INPUT = '\3';
const char KEY_GAMUT_INPUT = '\4';

const PROGMEM char INPUT_PROMPT[] = "> ";

enum MenuMode
{
    ITEM_KEY, // default
    INTEGER,
    // GAMUT, //[1-9:a-f;]
};
struct Menu
{
    char key;
    StringDescriptor title;
    const byte (*getSubmenuCase)(const Menu /*RAM*/ *selection);
    ArrayDescriptor<ArrayDescriptor<ArrayDescriptor<Menu>>> submenuCases; // cases[concatenation[menuArrays[menus[]]]]
    ArrayDescriptor<Menu> (*handleSelection)(const void *selection);
};

template <uint16_t timeout>
class MenuNavigator
{
private:
    class PtrMenu
    {
#ifdef DUMP_STACK_CHANGES
        friend class MenuNavigator;
#endif
    private:
        // ToDo: rework via static map FAR ptr -> NEAR addr
        /*
            const Menu *ref = nullptr;
            byte refCount = 0;
        */
    public:
        char menuKey;
        StringDescriptor menuTitle;
        PointerDescriptor<Menu> des;

        // PtrMenu(char key = '\0', StringDescriptor title = {}, PtrType pointerType = NONE, const Menu *pointer = nullptr) : menuKey(key), menuTitle(title), des{pointerType, pointer} {}

        const Menu *acquireMenu()
        {
            /**
            if (ref == nullptr)
            {
            /*/
            const Menu *ref;
            /**/
            if (des.type == FAR)
            {
                ref = (const Menu *)malloc(sizeof(Menu));
                memcpy_P(ref, des.ptr, sizeof(Menu));
            }
            else
            {
                ref = des.ptr;
            }
            /*
            }
            refCount++;
            */
            return ref;
        }

        void releaseMenu(/**/ const Menu *ref /**/)
        {
            /*
            if (refCount)
            {
                if (!--refCount)
                {
            */
            if (des.type == FAR)
            {
                free(ref);
            }
            /*
                    ref = nullptr;
                }
            }
            */
        }
    };

    class MenuStack
    {
#ifdef DUMP_STACK_CHANGES
        friend class MenuNavigator;
#endif
        PtrMenu current;
        MenuStack *prev;

        MenuStack(MenuStack *stack) : current(stack->current), prev(stack->prev){};
        void releasePointer()
        {
            // FixMe: align logic with «new» invocation
            /*
            if (current.des.type == NEAR)
            {
                delete current.des.ptr;
            }
            */
        }
        void cleanup()
        {
            if (prev)
            {
                releasePointer();
                prev->cleanup();
            }
            delete this;
        }

    public:
        MenuStack() : current{}, prev(nullptr){};

        void clear()
        {
            if (prev)
            {
                prev->cleanup();
            }
            current = {};
            prev = nullptr;
        }

        void push(const PtrMenu *menuPtr)
        {
            if (current.des.ptr)
            {
                prev = new MenuStack(this);
            }
            current = *menuPtr;
        }

        PtrMenu pop()
        {
            releasePointer();
            if (prev)
            {
                MenuStack *p = prev;
                current = prev->current;
                prev = prev->prev;
                delete p;
            }
            else
            {
                current = {};
            }
            return current;
        }
    } _stack;

    MenuMode _menuMode = ITEM_KEY;
    char _inputBuffer[255]{};
    byte _cmdLen = 0;
    ArrayDescriptor<PtrMenu> _currentMenu{};
    uint32_t _timestamp = 0;

    const /*PROGMEM*/ Menu *_menu;
    Stream &_stream;

    void goBack(byte times = 1)
    {
        _menuMode = ITEM_KEY;
        PtrMenu prev;
        for (int i = times; i >= 0; i--)
        {
            PtrMenu popped = _stack.pop();
#ifdef DUMP_STACK_CHANGES
            _stream.print(F("popped "));
            _stream.print(times - i);
            _stream.print(F("/"));
            _stream.print(times);
            _stream.print(F(", current type "));
            _stream.print(_stack.current.des.type);
            _stream.print(F(" @"));
            _stream.println((uint16_t)_stack.current.des.ptr);
#endif
            if (i == 1)
            {
                prev = popped;
            }
            if (i >= 1 && popped.des.type == NONE)
            {
                resetMenu();
                return;
            }
        }
        // ToDo: prev NEAR ptr may already be released after inner pop() if any
#ifdef DUMP_STACK_CHANGES
        _stream.print(F("pushing back type "));
        _stream.print(prev.des.type);
        _stream.print(F(" @"));
        _stream.println((uint16_t)prev.des.ptr);
#endif
        setMenu(&prev);
    }

    void appendMenu(const ArrayDescriptor<Menu> &source, byte &autoKey)
    {
        byte startLen = _currentMenu.len;
        byte newLen = _currentMenu.len + source.len;
        PtrMenu *array = (PtrMenu *)realloc(_currentMenu.des.ptr, sizeof(PtrMenu[newLen]));
        _currentMenu = {newLen, NEAR, array};
        for (byte menuItem = 0; menuItem < source.len; menuItem++)
        {
#ifdef DUMP_STACK_CHANGES
            _stream.print(F("\titem "));
            _stream.print(menuItem);
            _stream.print(F(" type "));
            _stream.print(source.des.type);
            _stream.print(F(" @"));
            _stream.print((uint16_t)&source.des.ptr[menuItem]);
#endif
            Menu item = source[menuItem];
            switch (item.key)
            {
                // ToDo: allow commands to follow other commands/menus - consider len changed - rework via _currentMenu next pointer
            case KEY_RESET_MENU:
#ifdef DUMP_STACK_CHANGES
                _stream.println(F("\treset"));
#endif
                resetMenu();
                return;
            case KEY_POP_MENU:
#ifdef DUMP_STACK_CHANGES
                _stream.println(F("\tback"));
#endif
                goBack();
                return;
            case KEY_INT_INPUT:
                _menuMode = INTEGER;
                _stream.print((__FlashStringHelper *)INPUT_PROMPT);
                // add KEY_INT_INPUT into _currentMenu
            default:
#ifdef DUMP_STACK_CHANGES
                _stream.print(F(": "));
                _stream.println(item.title.getPrintable());
#endif
                array[startLen + menuItem] = {item.key == KEY_AUTO ? (char)autoKey++ : item.key, item.title, source.des.type, &source.des.ptr[menuItem]};
            }
        }
    }

    void resetMenu()
    {
        _menuMode = ITEM_KEY;
        _timestamp = 0;
        setMenu(nullptr);
        printPrompt();
    }

    void setMenu(const PtrMenu *ptrMenu)
    {
#ifdef DUMP_STACK_CHANGES
        _stream.print(F("setMenu type "));
        _stream.print(ptrMenu->des.type);
        _stream.print(F(" @"));
        _stream.println((uint16_t)ptrMenu->des.ptr);
#endif
        _stream.print(F("\e[H\e[J"));
        if (ptrMenu == nullptr)
        {
            Menu *m = (Menu *)malloc(sizeof(Menu));
            memcpy_P(m, _menu, sizeof(Menu));
            PtrMenu *pMenu = (PtrMenu *)realloc(_currentMenu.des.ptr, sizeof(PtrMenu[1]));
            *pMenu = {m->key, m->title, FAR, _menu}; // ToDo: support NEAR _menu
            _currentMenu = {1, NEAR, pMenu};
            _stack.clear();
            free(m);
        }
        else
        {
            const Menu *m = ptrMenu->acquireMenu();
#ifdef DUMP_STACK_CHANGES
            _stream.print(F("Acquired. ref: "));
            _stream.println((uint16_t)m);
#endif
            byte menuCase = m->getSubmenuCase != nullptr
                                ? m->getSubmenuCase(m)
                                : 0;

            void *val;
            switch (_menuMode)
            {
            case INTEGER:
                // case GAMUT:
                val = _inputBuffer;
                break;
            default:
                val = nullptr;
                break;
            }
            ArrayDescriptor<Menu> dynamicMenu = m->handleSelection != nullptr
                                                    ? m->handleSelection(val)
                                                    : (ArrayDescriptor<Menu>){};
            switch (_menuMode)
            {
            case INTEGER:
                // case GAMUT:
                _cmdLen = 0;
            }

            byte autoKey = '1';
            _currentMenu.len = 0;
            _stack.push(ptrMenu);
#ifdef DUMP_STACK_CHANGES
            _stream.print(F("pushed type "));
            _stream.print(_stack.current.des.type);
            _stream.print(F(" @"));
            _stream.println((uint16_t)_stack.current.des.ptr);
#endif
            ArrayDescriptor<ArrayDescriptor<Menu>> theCase = m->submenuCases[menuCase];
            for (byte block = 0; block < theCase.len; block++)
            {
#ifdef DUMP_STACK_CHANGES
                _stream.print(F("block "));
                _stream.println(block);
#endif
                appendMenu(theCase[block], autoKey);
            }

            if (dynamicMenu.len > 0)
            {
#ifdef DUMP_STACK_CHANGES
                _stream.print(F("dynamic of len "));
                _stream.println(dynamicMenu.len);
#endif
                appendMenu(dynamicMenu, autoKey);
            }
            ptrMenu->releaseMenu(m);
#ifdef DUMP_STACK_CHANGES
            _stream.print(F("Released. ref: "));
            _stream.println((uint16_t)m);
#endif
        }
    }

    void printPrompt()
    {
        _stream.print(F("Press '"));
        // _stream.print(_currentMenu[0].menuKey);
        _stream.print(_currentMenu.des.ptr->menuKey); // ToDo: support FAR pointer without stack overflow
        _stream.println(F("' for menu or esc anywhere to quit"));
    }

public:
    MenuNavigator(Stream &ioStream, const /*PROGMEM*/ Menu &menu) : _menu(&menu), _stream(ioStream)
    {
        resetMenu();
    };

    void processMenu()
    {
        if (_timestamp > 0 && millis() > _timestamp + timeout)
        {
            resetMenu();
        }
        if (_stream.available())
        {
            _timestamp = millis();
            char keyRead = _stream.read();
            bool skipLookup = false;
            if (_currentMenu[0].des.ptr == _menu)
            {
                if (keyRead != _currentMenu[0].menuKey)
                {
                    printPrompt();
                    return;
                }
            }
            else
            {
                bool proceed = false;
                switch (_menuMode)
                {
                case INTEGER:
                    switch (keyRead)
                    {
                    case '\e':
                        if (_cmdLen)
                        {
                            _cmdLen = 0;
                            _stream.print(F("\e[G"));
                            _stream.print((__FlashStringHelper *)INPUT_PROMPT);
                            _stream.print(F("\e[K"));
                        }
                        else
                        {
                            resetMenu();
                        }
                        // ToDo: process esc-sequences such as arrows

                        break;
                    case '\x8':  // BS = c-h
                    case '\x7f': // BS = c-?
                        if (_cmdLen)
                        {
                            _cmdLen--;
                            _stream.print(F("\e[D\e[K"));
                        }
                        else
                        {
                            goBack();
                            skipLookup = true;
                            proceed = true;
                        }
                        break;
                    case '\xd':
                    {
                        _inputBuffer[_cmdLen] = '\0';
                        proceed = true;
                        break;
                    }
                    default:
                        if (_cmdLen < 255 && keyRead >= '0' && keyRead <= '9')
                        {
                            _stream.print(keyRead);
                            _inputBuffer[_cmdLen++] = keyRead;
                        }
                    }
                    if (proceed)
                    {
                        break;
                    }
                    return;
                default:
                    switch (keyRead)
                    {
                    case '\xa':
                        return;
                    case '\e':
                        resetMenu();
                        return;
                    case '\x8':  // BS = c-h
                    case '\x7f': // BS = c-?
                        goBack();
                        skipLookup = true;
                        // proceed to menu printout
                    }
                }
            }

            if (!skipLookup)
            {
                bool found = false;
                for (byte i = 0; i < _currentMenu.len; i++)
                {
                    PtrMenu menu = _currentMenu[i];
                    if (keyRead == menu.menuKey)
                    {
                        setMenu(&menu);
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    _stream.print(F("unknown option '"));
                    _stream.print(keyRead);
                    _stream.print(F("'(0x"));
                    _stream.print(keyRead, HEX);
                    _stream.println(F("), choose a valid menu item"));
                }
            }
            if (_menuMode == ITEM_KEY                         // not input mode
                && _currentMenu[0].menuTitle.des.type != NONE // not _menu or other invalid menu
            )
            {
                for (byte i = 0; i < _currentMenu.len; i++)
                {
                    _stream.print(_currentMenu[i].menuKey);
                    _stream.print(F(". "));
                    _stream.println(_currentMenu[i].menuTitle.getPrintable());
                }
                _stream.println();
            }
        }
    };
};