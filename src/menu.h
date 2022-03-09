#pragma once
#include <Stream.h>
#include "types.h"

const char KEY_AUTO = '\0';
const char KEY_RESET_MENU = '\1';
const char KEY_POP_MENU = '\2';
const char KEY_INPUT = '\3';

struct Menu
{
    char key;
    StringDescriptor title;
    const byte (*getSubmenuCase)(const Menu /*RAM*/ *selection);
    ArrayDescriptor<ArrayDescriptor<ArrayDescriptor<Menu>>> submenuCases; // cases[concatenation[menuArrays[menus[]]]]
    ArrayDescriptor<Menu> (*handleSelection)(const Menu /*original (PROGMEM/RAM)*/ *selection);
};

template <uint16_t timeout>
class MenuNavigator
{
private:
    struct PtrMenu
    {
        char menuKey;
        StringDescriptor menuTitle;
        PointerDescriptor<Menu> des;
    };

    class MenuStack
    {
        friend class MenuNavigator; // for DEBUG

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

    ArrayDescriptor<PtrMenu> _currentMenu{};
    // ArrayDescriptor<Menu> _dynamicMenu{};
    byte _selectedCase;
    uint32_t _timestamp = 0;

    const /*PROGMEM*/ Menu *_menu;
    Stream &_stream;

    void goBack()
    {
        PtrMenu prev = _stack.pop();
        /*
                // DEBUG
                _stream.print(F("outter popped, current type "));
                _stream.print(_stack.current.des.type);
                _stream.print(F(" @"));
                _stream.println((uint16_t)_stack.current.des.ptr);
        */
        if (prev.des.type != NONE)
        {
            _stack.pop();
            /*
                        // DEBUG
                        _stream.print(F("inner popped, current type "));
                        _stream.print(_stack.current.des.type);
                        _stream.print(F(" @"));
                        _stream.println((uint16_t)_stack.current.des.ptr);
            */
        }
        else
        {
            resetMenu();
            return;
        }
        // ToDo: prev NEAR ptr may already be released after inner pop() if any
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
            /*
            // DEBUG
            _stream.print(F("\titem "));
            _stream.print(menuItem);
            */
            Menu item = source[menuItem];
            switch (item.key)
            {
            case KEY_RESET_MENU:
                /*
                    // DEBUG
                    _stream.println(F("\treset"));
                */
                resetMenu();
                return; // ToDo: allow subsequent menus/commands - consider len changed - rework via current next pointer
            case KEY_POP_MENU:
                /*
                    // DEBUG
                    _stream.println(F("\tback"));
                */
                goBack();
                return; // ToDo: allow subsequent menus/commands - consider len changed - rework via current next pointer
            case KEY_INPUT:
                // no items, just consider in keyRead analysis
                return;
            default:
                /*
                    // DEBUG
                    _stream.print(F(": "));
                    _stream.println(item.title.getPrintable());
                */
                array[startLen + menuItem] = {item.key == KEY_AUTO ? (char)autoKey++ : item.key, item.title, source.des.type, &source.des.ptr[menuItem]};
            }
        }
    }

    void resetMenu()
    {
        _timestamp = 0;
        setMenu(nullptr);
        printPrompt();
    }

    void setMenu(const PtrMenu *ptrMenu)
    {
        /*
        // DEBUG
        _stream.print(F("setMenu type "));
        _stream.print(ptrMenu->des.type);
        _stream.print(F(" @"));
        _stream.println((uint16_t)ptrMenu->des.ptr);
        */
        if (ptrMenu == nullptr)
        {
            Menu *m = (Menu *)malloc(sizeof(Menu));
            memcpy_P(m, _menu, sizeof(Menu));
            PtrMenu *pMenu = (PtrMenu *)realloc(_currentMenu.des.ptr, sizeof(PtrMenu));
            *pMenu = {m->key, m->title, FAR, _menu};
            _currentMenu = {1, NEAR, pMenu};
            _stack.clear();
            free(m);
        }
        else
        {
            const Menu *m;
            if (ptrMenu->des.type == FAR)
            {
                m = (const Menu *)malloc(sizeof(Menu));
                memcpy_P(m, ptrMenu->des.ptr, sizeof(Menu));
            }
            else
            {
                m = ptrMenu->des.ptr;
            }
            byte menuCase = m->getSubmenuCase != nullptr
                                ? m->getSubmenuCase(m)
                                : 0;
            ArrayDescriptor<Menu> dynamicMenu = m->handleSelection != nullptr
                                                    ? m->handleSelection(ptrMenu->des.ptr)
                                                    : (ArrayDescriptor<Menu>){};

            byte autoKey = '1';
            _currentMenu.len = 0;
            _stack.push(ptrMenu);
            /*
            // DEBUG
            _stream.print(F("pushed type "));
            _stream.print(_stack.current.des.type);
            _stream.print(F(" @"));
            _stream.println((uint16_t)_stack.current.des.ptr);
            */
            ArrayDescriptor<ArrayDescriptor<Menu>> theCase = m->submenuCases[menuCase];
            for (byte block = 0; block < theCase.len; block++)
            {
                /*
                // DEBUG
                _stream.print(F("block "));
                _stream.println(block);
                */
                appendMenu(theCase[block], autoKey);
            }

            if (dynamicMenu.len > 0)
            {
                /*
                // DEBUG
                _stream.print(F("dynamic of len "));
                _stream.println(dynamicMenu.len);
                */
                appendMenu(dynamicMenu, autoKey);
            }

            if (ptrMenu->des.type == FAR)
            {
                free(m);
            }
        }
    }

    void printPrompt()
    {
        _stream.print(F("Press '"));
        _stream.print(_currentMenu[0].menuKey);
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
                /*
                if(isInput){
                    //ToDo: process esc-sequences like arrows in INPUT mode

                }
                */
                switch (keyRead)
                {
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

            if (!skipLookup)
            {
                bool found = false;
                for (byte i = 0; i < _currentMenu.len; i++)
                {
                    if (keyRead == _currentMenu[i].menuKey)
                    {
                        PtrMenu menu = _currentMenu[i];
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
            if (_currentMenu[0].menuTitle.des.type != NONE)
            { // main menu and control commands
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