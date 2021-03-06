#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/WaterObject.h"
#include "../S5/S5.h"
#include "../StationManager.h"
#include "../Things/ThingManager.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "ToolbarTopCommon.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Editor
{
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<uint8_t, 0x009C870C> last_town_option;

    namespace Widx
    {
        enum
        {
            map_generation_menu = Common::Widx::w6,
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 30, 28 }, widget_type::wt_7, 0),   // 0
        makeWidget({ 30, 0 }, { 30, 28 }, widget_type::wt_7, 0),  // 1
        makeWidget({ 74, 0 }, { 30, 28 }, widget_type::wt_7, 1),  // 2
        makeWidget({ 104, 0 }, { 30, 28 }, widget_type::wt_7, 1), // 3
        makeWidget({ 134, 0 }, { 30, 28 }, widget_type::wt_7, 1), // 4

        makeWidget({ 267, 0 }, { 30, 28 }, widget_type::wt_7, 2), // 5
        makeWidget({ 267, 0 }, { 30, 28 }, widget_type::wt_7, 2), // 6
        makeWidget({ 357, 0 }, { 30, 28 }, widget_type::wt_7, 2), // 7
        makeWidget({ 0, 0 }, { 1, 1 }, widget_type::none, 0),     // 8
        makeWidget({ 0, 0 }, { 1, 1 }, widget_type::none, 0),     // 9

        makeWidget({ 0, 0 }, { 1, 1 }, widget_type::none, 0),     // 10
        makeWidget({ 0, 0 }, { 1, 1 }, widget_type::none, 0),     // 11
        makeWidget({ 460, 0 }, { 30, 28 }, widget_type::wt_7, 3), // 12
        widgetEnd(),
    };

    static window_event_list _events;

    static void onMouseDown(window* window, widget_index widgetIndex);
    static void onDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    static void prepareDraw(window* window);

    static void initEvents()
    {
        _events.on_resize = Common::onResize;
        _events.event_03 = onMouseDown;
        _events.on_mouse_down = onMouseDown;
        _events.on_dropdown = onDropdown;
        _events.on_update = Common::onUpdate;
        _events.prepare_draw = prepareDraw;
        _events.draw = Common::draw;
    }

    // 0x0043CC2C
    void open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            Gfx::ui_size_t(Ui::width(), 28),
            WindowFlags::stick_to_front | WindowFlags::transparent | WindowFlags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << Common::Widx::loadsave_menu) | (1 << Common::Widx::audio_menu) | (1 << Common::Widx::zoom_menu) | (1 << Common::Widx::rotate_menu) | (1 << Common::Widx::view_menu) | (1 << Common::Widx::terraform_menu) | (1 << Widx::map_generation_menu) | (1 << Common::Widx::road_menu) | (1 << Common::Widx::towns_menu);
        window->initScrollWidgets();
        window->colours[0] = Colour::grey;
        window->colours[1] = Colour::grey;
        window->colours[2] = Colour::grey;
        window->colours[3] = Colour::grey;

        auto skin = ObjectManager::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = skin->colour_12;
            window->colours[1] = skin->colour_13;
            window->colours[2] = skin->colour_14;
            window->colours[3] = skin->colour_15;
        }
    }

    // 0x0043D638
    static void loadsaveMenuMouseDown(window* window, widget_index widgetIndex)
    {
        Dropdown::add(0, StringIds::load_landscape);
        Dropdown::add(1, StringIds::save_landscape);
        Dropdown::add(2, 0);
        Dropdown::add(3, StringIds::menu_about);
        Dropdown::add(4, StringIds::options);
        Dropdown::add(5, StringIds::menu_screenshot);
        Dropdown::add(6, 0);
        Dropdown::add(7, StringIds::menu_quit_to_menu);
        Dropdown::add(8, StringIds::menu_exit_openloco);
        Dropdown::showBelow(window, widgetIndex, 9, 0);
        Dropdown::setHighlightedItem(1);
    }

    // 0x0043D695
    static void loadsaveMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                // Load Landscape
                GameCommands::do_21(0, 0);
                break;

            case 1:
                // Save Landscape
                call(0x0043D705);
                break;

            case 3:
                About::open();
                break;

            case 4:
                Options::open();
                break;

            case 5:
            {
                loco_global<uint8_t, 0x00508F16> screenshot_countdown;
                screenshot_countdown = 10;
                break;
            }

            case 7:
                // Return to title screen
                GameCommands::do_21(0, 1);
                break;

            case 8:
                // Exit to desktop
                GameCommands::do_21(0, 2);
                break;
        }
    }

    // 0x0043D789
    static void audioMenuMouseDown(window* window, widget_index widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_mute);
        Dropdown::showBelow(window, widgetIndex, 1, 0);

        if (!Audio::isAudioEnabled())
            Dropdown::setItemSelected(0);

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043D7C1
    static void audioMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                Audio::toggleSound();
                break;
        }
    }

    // 0x004402BC
    static void mapGenerationMenuMouseDown(window* window, widget_index widgetIndex)
    {
        Dropdown::add(0, StringIds::landscape_generation_options);
        Dropdown::showBelow(window, widgetIndex, 1, 0);
        Dropdown::setHighlightedItem(0);
    }

    // 0x004402DA
    static void mapGenerationMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                call(0x0043DA43);
                break;
        }
    }

    // 0x0043D541
    static void onMouseDown(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuMouseDown(window, widgetIndex);
                break;

            case Widx::map_generation_menu:
                mapGenerationMenuMouseDown(window, widgetIndex);
                break;

            default:
                Common::onMouseDown(window, widgetIndex);
                break;
        }
    }

    // 0x0043D5A6
    static void onDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::map_generation_menu:
                mapGenerationMenuDropdown(window, widgetIndex, itemIndex);
                break;

            default:
                Common::onDropdown(window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x0043D2F3
    static void prepareDraw(window* window)
    {
        uint32_t x = std::max(640, Ui::width()) - 1;

        Common::rightAlignTabs(window, x, { Common::Widx::towns_menu });
        x -= 11;
        Common::rightAlignTabs(window, x, { Common::Widx::road_menu, Widx::map_generation_menu, Common::Widx::terraform_menu });

        if (S5::getOptions().editorStep == 1)
        {
            window->widgets[Common::Widx::zoom_menu].type = widget_type::wt_7;
            window->widgets[Common::Widx::rotate_menu].type = widget_type::wt_7;
            window->widgets[Common::Widx::view_menu].type = widget_type::wt_7;
            window->widgets[Common::Widx::terraform_menu].type = widget_type::wt_7;
            window->widgets[Widx::map_generation_menu].type = widget_type::wt_7;
            window->widgets[Common::Widx::towns_menu].type = widget_type::wt_7;
            if (last_road_option != 0xFF)
            {
                window->widgets[Common::Widx::road_menu].type = widget_type::wt_7;
            }
            else
            {
                window->widgets[Common::Widx::road_menu].type = widget_type::none;
            }
        }
        else
        {
            window->widgets[Common::Widx::zoom_menu].type = widget_type::none;
            window->widgets[Common::Widx::rotate_menu].type = widget_type::none;
            window->widgets[Common::Widx::view_menu].type = widget_type::none;
            window->widgets[Common::Widx::terraform_menu].type = widget_type::none;
            window->widgets[Widx::map_generation_menu].type = widget_type::none;
            window->widgets[Common::Widx::road_menu].type = widget_type::none;
            window->widgets[Common::Widx::towns_menu].type = widget_type::none;
        }

        auto interface = ObjectManager::get<interface_skin_object>();
        if (!Audio::isAudioEnabled())
        {
            window->activated_widgets |= (1 << Common::Widx::audio_menu);
            window->widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_inactive, window->colours[0]);
        }
        else
        {
            window->activated_widgets &= ~(1 << Common::Widx::audio_menu);
            window->widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_active, window->colours[0]);
        }

        window->widgets[Common::Widx::loadsave_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_loadsave, 0);
        window->widgets[Common::Widx::zoom_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_zoom, 0);
        window->widgets[Common::Widx::rotate_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_rotate, 0);
        window->widgets[Common::Widx::view_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_view, 0);

        window->widgets[Common::Widx::terraform_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_terraform, 0);
        window->widgets[Widx::map_generation_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_map_generation, 0);
        window->widgets[Common::Widx::road_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque, 0);

        if (last_town_option == 0)
            window->widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_towns, 0);
        else
            window->widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_industries, 0);
    }
}
