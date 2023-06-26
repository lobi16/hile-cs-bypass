﻿/*
*	OXWARE developed by oxiKKK
*	Copyright (c) 2023
* 
*	This program is licensed under the MIT license. By downloading, copying, 
*	installing or using this software you agree to this license.
*
*	License Agreement
*
*	Permission is hereby granted, free of charge, to any person obtaining a 
*	copy of this software and associated documentation files (the "Software"), 
*	to deal in the Software without restriction, including without limitation 
*	the rights to use, copy, modify, merge, publish, distribute, sublicense, 
*	and/or sell copies of the Software, and to permit persons to whom the 
*	Software is furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included 
*	in all copies or substantial portions of the Software. 
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
*	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
*	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
*	IN THE SOFTWARE.
*/

#include "precompiled.h"

VarBoolean ui_menu_resizable("ui_menu_resizable", "This is a beta feature!", false);

// Note: moved here from the header file for faster compile times
const float CMenuStyle::k_rounding_factor = 20.0f;
const Vector2D CMenuStyle::k_menu_rect_size = { 600, 400 };
const float CMenuStyle::k_tab_select_width = 120.0f;
const float CMenuStyle::k_top_region_size_h = 50.0f;
const float CMenuStyle::k_menu_contents_padding = 10.0f;
const float CMenuStyle::k_menu_contents_padding_bottom = 5.0f;
const float CMenuStyle::k_bottom_reserved_rect_h = 15.0f;
const float CMenuStyle::k_bottom_right_timestamp_rightside_padding = 5.0f;
const float CMenuStyle::k_bottom_right_timestamp_bottomside_padding = 2.0f;
const Vector2D CMenuStyle::k_bottom_left_desc_padding = { 5.0f, 2.0f };
const float CMenuStyle::k_child_contents_rounding = 5.0f;
const Vector2D CMenuStyle::k_child_contents_padding = { 5.0f, 2.0f };
const float CMenuStyle::k_child_width = 210.0f;
const Vector2D CMenuStyle::k_unload_button_padding = { 15.0f, 15.0f };
const Vector2D CMenuStyle::k_unload_button_size = { 105.0f, 20.0f };
const Vector2D CMenuStyle::k_about_button_size = { 50.0f, 20.0f };

Vector2D CMenuStyle::calc_child_size(float height) { return { -1.0f, height }; }
float CMenuStyle::get_child_width_w_padding() { return CMenuStyle::k_child_width - CMenuStyle::k_child_contents_padding.x * 2; }

// use this instead of width begin -1.0f
Vector2D CMenuStyle::child_full_width(float height) { return { -1.0f - k_menu_contents_padding, height }; }

//---------------------------------------------------------------------------------------------------

std::array<TabCallbackFn, UIMENU_Max> CUIMenu::s_active_tab_callback_translation =
{
	{
		&CUIMenu::tab_blank, 				// UIMENU_Blank
		&CUIMenu::tab_ragebot, 				// UIMENU_RageBot
		&CUIMenu::tab_legitbot,				// UIMENU_LegitBot
		&CUIMenu::tab_antibot,				// UIMENU_AntiBot
		&CUIMenu::tab_waybot,				// UIMENU_WayBot
		&CUIMenu::tab_viewmodel,			// UIMENU_Viewmodel
		&CUIMenu::tab_world,				// UIMENU_World
		&CUIMenu::tab_render,				// UIMENU_Render
		&CUIMenu::tab_screen,				// UIMENU_Screen
		&CUIMenu::tab_exploits,				// UIMENU_Exploits
		&CUIMenu::tab_movement,				// UIMENU_Movement
		&CUIMenu::tab_miscellaneous2,		// UIMENU_Miscellaneous2
		&CUIMenu::tab_miscellaneous3,		// UIMENU_Miscellaneous3
		&CUIMenu::tab_config,				// UIMENU_Config
		&CUIMenu::tab_binds,				// UIMENU_Binds
		&CUIMenu::tab_incommands,			// UIMENU_InCommands
		&CUIMenu::tab_cmdlist,				// UIMENU_CommandList
		&CUIMenu::tab_varlist,				// UIMENU_VariableList
		&CUIMenu::tab_others,				// UIMENU_Others
	}
};

//---------------------------------------------------------------------------------------------------

void MenuTabItem::render(Vector2D& offset, Vector2D& relative_offset, EMenuTabId& active_tab_id)
{
	g_gui_widgets_i->set_cursor_pos(relative_offset);

	auto button_font = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText, FDC_Bold);
	g_gui_widgets_i->push_font(button_font);

	// text color
	g_gui_widgets_i->push_color(ImGuiCol_Text, g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextDark));

	bool selected = (active_tab_id == m_tab_id);
	if (g_gui_widgets_i->add_toggle_button(m_label.c_str(), { CMenuStyle::k_tab_select_width - 20.0f, 0.0f }, selected))
	{
		active_tab_id = m_tab_id; // make current one selected
	}

	if (g_gui_widgets_i->is_last_widget_hovered())
	{
		CMenuWidgetDescription::the().set_desc(m_desc.c_str());
	}

	g_gui_widgets_i->pop_color(1);

	g_gui_widgets_i->pop_font();

	relative_offset.y += button_font->FontSize + 4.0f;
	offset.y += button_font->FontSize + 4.0f;
}

//---------------------------------------------------------------------------------------------------

void MenuTabSection::render(Vector2D& offset, Vector2D& relative_offset, const Vector2D& child_size, EMenuTabId& active_tab_id)
{
	render_current_label(offset, relative_offset, child_size);

	for (auto& tab : m_menu_tabs)
	{
		tab.render(offset, relative_offset, active_tab_id);
	}
}

void MenuTabSection::render_current_label(Vector2D& offset, Vector2D& relative_offset, const Vector2D& child_size)
{
	auto section_label_font = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText, FDC_Regular);

	// section title
	g_gui_window_rendering_i->render_text(g_gui_window_rendering_i->get_current_drawlist(), 
										  section_label_font,
										  { offset.x, offset.y + 5.0f },
										  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextLight),
										  m_label);

	relative_offset.y += section_label_font->FontSize;
	offset.y += section_label_font->FontSize;

	// separator underneath
	g_gui_window_rendering_i->render_line(g_gui_window_rendering_i->get_current_drawlist(), 
										  { offset.x, offset.y + 4.0f },
										  { offset.x + child_size.x - 10.f * 2.f, offset.y + 5.f },
										  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_Separator));

	relative_offset.y += 8.f;
	offset.y += 8.f;
}

//---------------------------------------------------------------------------------------------------

void CUIMenu::on_initialize()
{
	//m_tabsec_AIHelpers.set_label("AI Helpers");
	//m_tabsec_AIHelpers.add_tab({ UIMENU_RageBot, &CUIMenu::tab_ragebot, "Ragebot", "TODO item description" });
	//m_tabsec_AIHelpers.add_tab({ UIMENU_LegitBot, &CUIMenu::tab_legitbot, "Legitbot", "TODO item description" });
	//m_tabsec_AIHelpers.add_tab({ UIMENU_AntiBot, &CUIMenu::tab_antibot, "Antibot", "TODO item description" });
	//m_tabsec_AIHelpers.add_tab({ UIMENU_WayBot, &CUIMenu::tab_waybot, "Waybot", "TODO item description" });
	//m_tab_sections.push_back(&m_tabsec_AIHelpers);

	m_tabsec_Visuals.set_label("Visuals");
	m_tabsec_Visuals.add_tab({ UIMENU_World, &CUIMenu::tab_world, "World", "World-related cheats" });
	m_tabsec_Visuals.add_tab({ UIMENU_Render, &CUIMenu::tab_render, "Render", "Rendering-related cheats" });
	m_tabsec_Visuals.add_tab({ UIMENU_Screen, &CUIMenu::tab_screen, "Screen", "Screen-related cheats" });
	m_tab_sections.push_back(&m_tabsec_Visuals);

	m_tabsec_Miscellaneous.set_label("Miscellaneous");
	m_tabsec_Miscellaneous.add_tab({ UIMENU_Exploits, &CUIMenu::tab_exploits, "Exploits", "Various game exploitations" });
	m_tabsec_Miscellaneous.add_tab({ UIMENU_Movement, &CUIMenu::tab_movement, "Movement", "Kreedz/Movement cheats" });
	//m_tabsec_Miscellaneous.add_tab({ UIMENU_Miscellaneous2, &CUIMenu::tab_miscellaneous2, "Miscellaneous2", "TODO item description" });
	//m_tabsec_Miscellaneous.add_tab({ UIMENU_Miscellaneous3, &CUIMenu::tab_miscellaneous3, "Miscellaneous3", "TODO item description" });
	m_tab_sections.push_back(&m_tabsec_Miscellaneous);

	m_tabsec_Configuration.set_label("Configuration");
	m_tabsec_Configuration.add_tab({ UIMENU_Config, &CUIMenu::tab_config, "Config", "Cheat configuration and configs" });
	m_tabsec_Configuration.add_tab({ UIMENU_Binds, &CUIMenu::tab_binds, "Binds", "Cheat bind manager" });
	m_tabsec_Configuration.add_tab({ UIMENU_InCommands, &CUIMenu::tab_incommands, "InCommands", "InCommands manager" });
	m_tab_sections.push_back(&m_tabsec_Configuration);

	m_tabsec_Other.set_label("Other");
	m_tabsec_Other.add_tab({ UIMENU_CommandList, &CUIMenu::tab_cmdlist, "Command list", "Cheat command list" });
	m_tabsec_Other.add_tab({ UIMENU_VariableList, &CUIMenu::tab_varlist, "Variable list", "Cheat variable list" });
	m_tabsec_Other.add_tab({ UIMENU_Others, &CUIMenu::tab_others, "Others", "Other miscelanoues items" });
	m_tab_sections.push_back(&m_tabsec_Other);
}

void CUIMenu::on_render()
{
	// we have to update it here since the keyscan buttons may be outside of the InCommands tab.
	CUIInCommandKeyBinding::the().update();

	g_gui_widgets_i->set_next_window_pos({ 100, 100 }, ImGuiCond_Once);
	g_gui_widgets_i->set_next_window_size(CMenuStyle::k_menu_rect_size, ImGuiCond_Once);
	g_gui_widgets_i->set_next_window_rounding(CMenuStyle::k_rounding_factor, ImDrawFlags_RoundCornersTopRight | ImDrawFlags_RoundCornersBottomLeft);

	g_gui_widgets_i->push_stylevar(ImGuiStyleVar_WindowMinSize, CMenuStyle::k_menu_rect_size);

	auto window_flags =
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav
		| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;

	if (ui_menu_resizable.get_value())
	{
		window_flags &= ~ImGuiWindowFlags_NoResize;
	}

	auto segoeui_extra = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.extra(1.1f), FDC_Bold);
	auto segoeui_small = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.small(), FDC_Bold);

	g_gui_widgets_i->push_stylevar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });

	CMenuWidgetDescription::the().clear_desc();

	g_gui_widgets_i->create_new_window(
		"menu", window_flags, [&]()
		{
			auto window_pos = g_gui_widgets_i->get_current_window_pos();
			auto window_size = g_gui_widgets_i->get_current_window_size();

			g_gui_widgets_i->set_next_window_rounding(CMenuStyle::k_rounding_factor, ImDrawFlags_RoundCornersBottomLeft);
			g_gui_widgets_i->add_child(
				"menu_left",
				Vector2D(CMenuStyle::k_tab_select_width, window_size.y),
				false,
				ImGuiWindowFlags_None,
				[&]()
				{
					auto child_pos = g_gui_widgets_i->get_current_window_pos();
					auto child_size = g_gui_widgets_i->get_current_window_size();

					auto label_size = g_gui_fontmgr_i->calc_font_text_size(segoeui_extra, "OXWARE");

					g_gui_window_rendering_i->render_text(g_gui_window_rendering_i->get_current_drawlist(), 
														  segoeui_extra,
														  { child_pos.x + child_size.x / 2 - label_size.x / 2, window_pos.y + 10 },
														  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextDark),
														  "OXWARE");

					m_sectab_relative_active_offset = { 10.0f, 50.0f };
					m_sectab_active_offs = child_pos + m_sectab_relative_active_offset;

					for (auto sec : m_tab_sections)
					{
						sec->render(m_sectab_active_offs, m_sectab_relative_active_offset,
									child_size, m_active_tab_id);
					}
				}
			);

			// menu and tab select separator
			g_gui_window_rendering_i->render_line(g_gui_window_rendering_i->get_current_drawlist(), 
												  { window_pos.x + CMenuStyle::k_tab_select_width, window_pos.y - 0.5f },
												  { window_pos.x + CMenuStyle::k_tab_select_width, window_pos.y + window_size.y },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_Separator));


			// top side menu separator
			g_gui_window_rendering_i->render_line(g_gui_window_rendering_i->get_current_drawlist(), 
												  { window_pos.x + CMenuStyle::k_tab_select_width, window_pos.y + CMenuStyle::k_top_region_size_h },
												  { window_pos.x + window_size.x, window_pos.y + CMenuStyle::k_top_region_size_h },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_Separator));

			//
			// render topside contents
			//
			auto topside_font = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.small(), FDC_Regular);

			auto version_label = std::format("Version: {}", OXVER_STRING);
			g_gui_window_rendering_i->render_text(g_gui_window_rendering_i->get_current_drawlist(), 
												  topside_font,
												  { window_pos.x + CMenuStyle::k_tab_select_width + 5.0f, window_pos.y + 0.0f + 3.0f },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextDark),
												  version_label);

			auto build_label = std::format("Build: {}", OX_BUILD);
			g_gui_window_rendering_i->render_text(g_gui_window_rendering_i->get_current_drawlist(),
												  topside_font,
												  { window_pos.x + CMenuStyle::k_tab_select_width + 5.0f, window_pos.y + 15.0f + 3.0f },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextDark),
												  build_label);

			auto compiletm_label = std::format("Compiled at: {}", OX_COMPILE_TIMESTAMP);
			g_gui_window_rendering_i->render_text(g_gui_window_rendering_i->get_current_drawlist(), 
												  topside_font,
												  { window_pos.x + CMenuStyle::k_tab_select_width + 5.0f, window_pos.y + 30.0f + 3.0f },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextDark), 
												  compiletm_label);

			//
			// now render contents of the menu.
			//

			g_gui_widgets_i->set_cursor_pos({
				CMenuStyle::k_tab_select_width + CMenuStyle::k_menu_contents_padding,
				CMenuStyle::k_top_region_size_h + CMenuStyle::k_menu_contents_padding });

			g_gui_widgets_i->add_child(
				"menu_contents",
				{
					window_size.x - CMenuStyle::k_tab_select_width - (CMenuStyle::k_menu_contents_padding * 2.0f),
					window_size.y - CMenuStyle::k_top_region_size_h - (CMenuStyle::k_menu_contents_padding * 1.0f + CMenuStyle::k_menu_contents_padding_bottom) - CMenuStyle::k_bottom_reserved_rect_h,
				},
				false,
				ImGuiWindowFlags_NoBackground,
				[&]()
				{
					auto tab_fn = s_active_tab_callback_translation[m_active_tab_id];
					(this->*tab_fn)();
				});

			// bottom separator
			g_gui_window_rendering_i->render_line(g_gui_window_rendering_i->get_current_drawlist(), 
												  { window_pos.x + CMenuStyle::k_tab_select_width + 1.0f, window_pos.y + (window_size.y - CMenuStyle::k_bottom_reserved_rect_h - 1.0f) },
												  { window_pos.x + window_size.x,  window_pos.y + (window_size.y - CMenuStyle::k_bottom_reserved_rect_h - 1.0f) },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_Separator));

			// bottom background
			g_gui_window_rendering_i->render_box(g_gui_window_rendering_i->get_current_drawlist(), 
												 { window_pos.x + CMenuStyle::k_tab_select_width + 1.0f, window_pos.y + (window_size.y - CMenuStyle::k_bottom_reserved_rect_h) },
												  { window_pos.x + window_size.x, window_pos.y + (window_size.y - 1.0f) },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_ChildBackground));

			// bottom right timestamp label
			auto timestamp_label = CStringTools::the().format_date(std::chrono::system_clock::now());
			auto timestamp_label_size = g_gui_fontmgr_i->calc_font_text_size(segoeui_small, timestamp_label.c_str());
			g_gui_window_rendering_i->render_text(g_gui_window_rendering_i->get_current_drawlist(), 
												  segoeui_small,
												  { 
													  window_pos.x + (window_size.x - CMenuStyle::k_bottom_right_timestamp_rightside_padding - timestamp_label_size.x), 
													  window_pos.y + (window_size.y - CMenuStyle::k_bottom_right_timestamp_bottomside_padding - timestamp_label_size.y)
												  },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextDark),
												  timestamp_label);

			// bottom-left description
			auto desc_label = CMenuWidgetDescription::the().get_desc();
			auto desc_label_size = g_gui_fontmgr_i->calc_font_text_size(segoeui_small, desc_label);
			g_gui_window_rendering_i->render_text(g_gui_window_rendering_i->get_current_drawlist(),
												  segoeui_small,
												  {
													  window_pos.x + CMenuStyle::k_tab_select_width + CMenuStyle::k_bottom_left_desc_padding.x,
													  window_pos.y + (window_size.y - CMenuStyle::k_bottom_left_desc_padding.y - desc_label_size.y)
												  },
												  g_gui_thememgr_i->get_current_theme()->get_color(GUICLR_TextDark),
												 desc_label);
			
			// Unload button
			Vector2D unload_button_pos = Vector2D(window_size.x - CMenuStyle::k_unload_button_size.x - CMenuStyle::k_unload_button_padding.x, CMenuStyle::k_unload_button_padding.y);
			g_gui_widgets_i->set_cursor_pos(unload_button_pos);

			if (g_gui_widgets_i->add_button("manually unload", CMenuStyle::k_unload_button_size, false, BUTTONFLAG_CenterLabel))
			{
				// end next frame.
				CoXWARE::the().end_cheat_execution();
			}

			// About button
			Vector2D about_button_pos = unload_button_pos - Vector2D(CMenuStyle::k_about_button_size.x + 5.0f, 0.0f);
			g_gui_widgets_i->set_cursor_pos(about_button_pos);

			if (g_gui_widgets_i->add_button("about", CMenuStyle::k_about_button_size, false, BUTTONFLAG_CenterLabel))
			{
				COxWareUI::the().create_welcome_popup();
			}
		});

	auto screen = g_imgui_platform_layer_i->get_screen_size();

	auto font = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText, FDC_Regular);

	const char* label = "www.github.com/oxiKKK/oxware";
	auto label_size = g_gui_fontmgr_i->calc_font_text_size(font, label);

	g_gui_widgets_i->set_next_window_pos({ screen.x - label_size.x + 10.0f, screen.y - label_size.y }, ImGuiCond_Always);

	static constexpr auto window_flags1 = 
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | 
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoMove;
	g_gui_widgets_i->create_new_window(
		"github_link", window_flags1, 
		[&]()
		{
			if (g_gui_widgets_i->add_hypertext_link(label))
			{
				CGenericUtil::the().open_link_inside_browser(label);
			}
		});

	g_gui_widgets_i->pop_stylevar(); // window padding

	g_gui_widgets_i->pop_stylevar(); // min window size
}

void CUIMenu::on_destroy()
{
	CConsole::the().info(__FUNCTION__ "()");
}

//---------------------------------------------------------------------------------------------------

void CUIMenu::tab_blank()
{
	g_gui_widgets_i->add_window_centered_text_disabled("Start by looking through the tabs on the left!");
}

void CUIMenu::tab_ragebot()
{
}

void CUIMenu::tab_legitbot()
{
	g_gui_widgets_i->add_text(__FUNCTION__ "()");
}

void CUIMenu::tab_antibot()
{
	g_gui_widgets_i->add_text(__FUNCTION__ "()");
}

void CUIMenu::tab_waybot()
{
	g_gui_widgets_i->add_text(__FUNCTION__ "()");
}

void CUIMenu::tab_viewmodel()
{
	g_gui_widgets_i->add_text(__FUNCTION__ "()");
}

void CUIMenu::tab_world()
{
	if (g_gui_widgets_i->begin_columns(__FUNCTION__, 2))
	{
		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Removals", CMenuStyle::calc_child_size(250), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Remove screenshake", &remove_screenshake);
				CUIMenuWidgets::the().add_checkbox("Remove viewmodel", &remove_viewmodel);
				CUIMenuWidgets::the().add_checkbox("Remove MOTD", &remove_motd);
				CUIMenuWidgets::the().add_checkbox("Remove sniper scopes", &remove_sniper_scope);

				g_gui_widgets_i->add_separtor_with_text("Remove players");
				CUIMenuWidgets::the().add_checkbox("All", &remove_players_all);

				if (remove_players_all.get_value())
				{
					g_gui_widgets_i->push_disabled();
				}

				if (g_gui_widgets_i->begin_columns("player_removals", 2))
				{
					g_gui_widgets_i->goto_next_column();

					CUIMenuWidgets::the().add_checkbox("Enemy", &remove_players_enemy);
					CUIMenuWidgets::the().add_checkbox("Teammates", &remove_players_teammates);

					g_gui_widgets_i->goto_next_column();

					CUIMenuWidgets::the().add_checkbox("Ts", &remove_players_t);
					CUIMenuWidgets::the().add_checkbox("CTs", &remove_players_ct);

					g_gui_widgets_i->end_columns();
				}

				if (remove_players_all.get_value())
				{
					g_gui_widgets_i->pop_disabled();
				}

				g_gui_widgets_i->add_spacing();
				CUIMenuWidgets::the().add_description_text("In order to disable in-game fog, use \"gl_fog\" command.");

				g_gui_widgets_i->add_separtor_with_text("Remove HUD");

				CUIMenuWidgets::the().feature_enabled_section(
				&remove_hud_enable,
				[]()
				{
					g_gui_widgets_i->add_padding({ 0, 5.0f });

					if (g_gui_widgets_i->begin_columns("removals_hud", 2))
					{
						g_gui_widgets_i->goto_next_column();
						CUIMenuWidgets::the().add_checkbox("Weapons", &remove_hud_weapons);
						CUIMenuWidgets::the().add_checkbox("Crosshair", &remove_hud_crosshair);
						CUIMenuWidgets::the().add_checkbox("Flashlight", &remove_hud_flashlight);

						g_gui_widgets_i->goto_next_column();

						CUIMenuWidgets::the().add_checkbox("Health", &remove_hud_health);
						CUIMenuWidgets::the().add_checkbox("Timer", &remove_hud_timer);
						CUIMenuWidgets::the().add_checkbox("Money", &remove_hud_money);

						g_gui_widgets_i->end_columns();
					}
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Thirdperson", CMenuStyle::calc_child_size(100), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				g_gui_widgets_i->add_spacing();
				g_gui_widgets_i->add_spacing();

				CUIMenuWidgets::the().add_slider("Distance", "%0.0f units", &thirdperson_dist);
				CUIMenuWidgets::the().add_checkbox("Block wall", &thirdperson_block_wall);
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Anti demo recorder", CMenuStyle::calc_child_size(120), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Enable", &anti_demo_recorder_enable);

				CUIMenuWidgets::the().add_description_text(
					"If enabled, server will not be able to record demos on your game.\n"
					"You'll be notified every time server wants to start a demo inside the console.");
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Anti screen", CMenuStyle::calc_child_size(90), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&antiscreen_enable,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Frames to wait", "%0.0f second(s)", &antiscreen_time);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Automation", CMenuStyle::calc_child_size(120), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Auto reload", &auto_reload);
				CUIMenuWidgets::the().add_checkbox("Auto pistol", &auto_pistol);
				CUIMenuWidgets::the().add_checkbox("Auto zoom", &auto_zoom);
			});

		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Viewmodel offset", CMenuStyle::calc_child_size(100), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&viewmodel_offset_enable,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Amount", "%0.1f", &viewmodel_offset_value);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Smoke visuals", CMenuStyle::calc_child_size(180), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&smoke_visuals,
				[]()
				{
					CUIMenuWidgets::the().add_color_edit("Color", &smoke_color);
					CUIMenuWidgets::the().add_checkbox("Rainbow smoke", &smoke_rainbow);

					CUIMenuWidgets::the().add_slider("Opacity", "%0.0f %%", &smoke_opacity);

					CUIMenuWidgets::the().add_description_text("Everything changed here will take effect only on new smoke creation.");
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Non-steam fps fix", CMenuStyle::calc_child_size(150), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Enable", &nonsteam_fps_fix);

				CUIMenuWidgets::the().add_description_text(
					"This fixes the FPS movement dependence, where with higher fps you would get slomotion movement and \"higher\" gravity."
					"\nFor more technical details, follow this link:");

				g_gui_widgets_i->push_font(g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.small(), FDC_Regular));
				if (g_gui_widgets_i->add_hypertext_link("github.com/ValveSoftware/halflife/issues/1940"))
				{
					CGenericUtil::the().open_link_inside_browser("https://github.com/ValveSoftware/halflife/issues/1940");
				}
				g_gui_widgets_i->pop_font();
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"World visuals", CMenuStyle::calc_child_size(120), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&world_visuals_enable,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Dimlight", "%0.0f %%", &world_visuals_dimlight);

					CUIMenuWidgets::the().add_checkbox("Rainbow world", &world_visuals_rainbow);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Environmental effects", CMenuStyle::calc_child_size(300), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&env_enable,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Generate radius", "%0.0f units", &env_radius);

					g_gui_widgets_i->add_separtor_with_text("Ground fog");
					CUIMenuWidgets::the().feature_enabled_section(
					&env_ground_fog,
					[]()
					{
						CUIMenuWidgets::the().add_slider("Density", "%0.0f", &env_ground_fog_density);
					});
					
					g_gui_widgets_i->begin_tab("env_effects", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll);

					float tab_height = -1.0f;

					g_gui_widgets_i->add_tab_item(
						"Rain", false,
						{ -1.0f, tab_height },
						[]()
						{
							CUIMenuWidgets::the().feature_enabled_section(
							&env_rain,
							[]()
							{
								CUIMenuWidgets::the().add_slider("Density", "%0.1f", &env_rain_density);
								CUIMenuWidgets::the().add_checkbox("Ambient sound", &env_rain_ambient);
								CUIMenuWidgets::the().add_checkbox("Occasional Thunder", &env_rain_ambient_thunder);
							});
						});

					g_gui_widgets_i->add_tab_item(
						"Snow", false,
						{ -1.0f, tab_height },
						[]()
						{
							CUIMenuWidgets::the().feature_enabled_section(
							&env_snow,
							[]()
							{
								CUIMenuWidgets::the().add_slider("Density", "%0.1f", &env_snow_density);
								CUIMenuWidgets::the().add_slider("Flake size", "%0.0fx", &env_snow_flake_size);
							});
						});

					g_gui_widgets_i->end_tab();
				});
			});

		g_gui_widgets_i->end_columns();
	}
}

void CUIMenu::tab_render()
{
	if (g_gui_widgets_i->begin_columns(__FUNCTION__, 2))
	{
		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Field of view", CMenuStyle::calc_child_size(100), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&custom_fov,
				[]()
				{
					CUIMenuWidgets::the().add_slider("FOV scale", "%0.1fx", &custom_fov_value);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Aspect ratio", CMenuStyle::calc_child_size(100), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&aspect_ratio_enable,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Aspect ratio", "%0.1f", &aspect_ratio_value);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Studio renderer", CMenuStyle::calc_child_size(235), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				g_gui_widgets_i->add_padding({ 0, 5.0f });
				g_gui_widgets_i->add_separtor_with_text("Player skeleton");

				CUIMenuWidgets::the().add_checkbox("Enable ##skelly", &mdlchams_player_skeleton);

				g_gui_widgets_i->add_padding({ 0, 5.0f });
				g_gui_widgets_i->add_separtor_with_text("Player head hitbox");

				CUIMenuWidgets::the().add_checkbox_with_color("Enable ##hbox", &mdlchams_head_box_enable, &mdlchams_head_box_color);

				g_gui_widgets_i->add_padding({ 0, 5.0f });
				g_gui_widgets_i->add_separtor_with_text("Other");

				CUIMenuWidgets::the().add_checkbox("Real player model ##skelly", &mdlchams_render_real_playermodel,
												   "Renders \"Real playermodel\". Hitboxes of this playermodel are used for hit registration, no matter what the acutal model is.");

				CUIMenuWidgets::the().add_description_text_ex("You can see this why this is useful here:", nullptr, true);
				g_gui_widgets_i->push_font(g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.small(), FDC_Regular));
				if (g_gui_widgets_i->add_hypertext_link("https://youtu.be/xMd9m3McNvo"))
				{
					CGenericUtil::the().open_link_inside_browser("https://youtu.be/xMd9m3McNvo");
				}
				g_gui_widgets_i->pop_font();

				g_gui_widgets_i->add_padding({ 0, 5.0f });
				CUIMenuWidgets::the().add_checkbox("Disable animations", &mdlchams_disable_animations);
				CUIMenuWidgets::the().add_checkbox("Enforce default VM", &mdlchams_force_default_viewmodel);
			});

		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Model chams", CMenuStyle::calc_child_size(250), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&mdlchams_enable,
				[]()
				{
					g_gui_widgets_i->begin_tab("model_chams_tab", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll);

					float tab_height = 80.0f;

					g_gui_widgets_i->add_tab_item(
						"VM", false,
						{ -1.0f, tab_height },
						[]()
						{
							g_gui_widgets_i->add_padding({ 0, 5.0f });
							g_gui_widgets_i->add_separtor_with_text("Viewmodel");
							CUIMenuWidgets::the().add_checkbox_with_color("Enable ##VM", &mdlchams_viewmodel_enable, &mdlchams_viewmodel_color);
							CUIMenuWidgets::the().add_listbox("Type ##VM", &mdlchams_viewmodel_type, { "Flat", "Shaded" });
						});

					g_gui_widgets_i->add_tab_item(
						"T", false,
						{ -1.0f, tab_height },
						[]()
						{
							g_gui_widgets_i->add_padding({ 0, 5.0f });
							g_gui_widgets_i->add_separtor_with_text("Terrorists");
							CUIMenuWidgets::the().add_checkbox_with_color("Enable ##T", &mdlchams_players_t_enable, &mdlchams_players_t_color);
							CUIMenuWidgets::the().add_listbox("Type ##T", &mdlchams_players_t_type, { "Flat", "Shaded" });
						});

					g_gui_widgets_i->add_tab_item(
						"CT", false,
						{ -1.0f, tab_height },
						[]()
						{
							g_gui_widgets_i->add_padding({ 0, 5.0f });
							g_gui_widgets_i->add_separtor_with_text("Counter-Terrorists");
							CUIMenuWidgets::the().add_checkbox_with_color("Enable ##CT", &mdlchams_players_ct_enable, &mdlchams_players_ct_color);
							CUIMenuWidgets::the().add_listbox("Type ##CT", &mdlchams_players_ct_type, { "Flat", "Shaded" });
						});

					g_gui_widgets_i->end_tab();

					g_gui_widgets_i->add_padding({ 0, 5.0f });
					g_gui_widgets_i->add_separtor_with_text("Properties");
					CUIMenuWidgets::the().add_checkbox("Flat-shaded", &mdlchams_flatshaded);
					CUIMenuWidgets::the().add_checkbox("Blend", &mdlchams_blend);

					g_gui_widgets_i->add_spacing();
					CUIMenuWidgets::the().add_checkbox("Rainbow colors", &mdlchams_rainbow);
					CUIMenuWidgets::the().add_slider("Speed", "%0.0fx", &mdlchams_rainbow_speed);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"HUD rendering", CMenuStyle::calc_child_size(210), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				g_gui_widgets_i->add_padding({ 0, 5.0f });
				g_gui_widgets_i->add_separtor_with_text("Custom rendering");

				CUIMenuWidgets::the().feature_enabled_section(
				&hud_render,
				[]()
				{
					CUIMenuWidgets::the().add_checkbox("Current weapon", &hud_render_current_weapon);
					CUIMenuWidgets::the().add_checkbox("Velocity", &hud_render_velocity);
				});

				g_gui_widgets_i->add_padding({ 0, 5.0f });
				g_gui_widgets_i->add_separtor_with_text("Colors");

				CUIMenuWidgets::the().add_checkbox_with_color("HUD Color", &hud_color_enable, &hud_color);
			});

		g_gui_widgets_i->end_columns();
	}
}

void CUIMenu::tab_screen()
{
	if (g_gui_widgets_i->begin_columns(__FUNCTION__, 2))
	{
		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Flashbang fade", CMenuStyle::calc_child_size(100), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&flashfademod_enable,
				&flashfademod_color,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Fade factor", "%0.1f", &flashfademod_fade_factor);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"ESP", CMenuStyle::calc_child_size(300), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&esp_enable,
				[]()
				{
					g_gui_widgets_i->add_spacing();

					CUIMenuWidgets::the().add_checkbox("Box background", &esp_background);
					CUIMenuWidgets::the().add_checkbox("Box outline", &esp_box_outline);
					CUIMenuWidgets::the().add_listbox("Box type", &esp_box_type, { "Normal", "Corners" });

					if (g_gui_widgets_i->begin_columns("esp_player", 2))
					{
						g_gui_widgets_i->goto_next_column();
						CUIMenuWidgets::the().add_checkbox("Enemy", &esp_player_enemy);
						g_gui_widgets_i->goto_next_column();
						CUIMenuWidgets::the().add_checkbox("Teammates", &esp_player_teammates);

						g_gui_widgets_i->end_columns();
					}

					g_gui_widgets_i->begin_tab("esp_tab", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll);

					float tab_height = -1.0f;

					g_gui_widgets_i->add_tab_item(
						"Players", false,
						{ -1.0f, tab_height },
						[]()
						{
							CUIMenuWidgets::the().add_checkbox("Enable ##player", &esp_player_enable);
							CUIMenuWidgets::the().add_checkbox("Show name", &esp_player_name);
						});

					g_gui_widgets_i->add_tab_item(
						"Entities", false,
						{ -1.0f, tab_height },
						[]()
						{
							CUIMenuWidgets::the().add_checkbox("Enable ##ents", &esp_entity_enable);
						});

					g_gui_widgets_i->add_tab_item(
						"Sound", false,
						{ -1.0f, tab_height },
						[]()
						{
							CUIMenuWidgets::the().add_checkbox("Enable ##sound", &esp_sound_enable);
							CUIMenuWidgets::the().add_slider("Display life", "%0.1f seconds", &esp_sound_interval);
							CUIMenuWidgets::the().add_checkbox("Filter local", &esp_sound_filter_local);
							CUIMenuWidgets::the().add_listbox("Type ##hidden", &esp_sound_type, { "2D", "3D" });
							
							CUIMenuWidgets::the().feature_enabled_section(
							&esp_sound_resolver,
							[]()
							{
								CUIMenuWidgets::the().add_slider("Resolver dist", "%0.0f units", &esp_sound_resolver_distace);
							}, "Resolver");
						});

					g_gui_widgets_i->end_tab();
				});
			});

		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Custom vanilla crosshair", CMenuStyle::calc_child_size(270), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&crosshair_enable,
				[]()
				{
					CUIMenuWidgets::the().add_checkbox("Dynamic", &crosshair_dynamic);
					CUIMenuWidgets::the().add_checkbox("Translucent", &crosshair_translucent);
					CUIMenuWidgets::the().add_checkbox("Static", &crosshair_static);

					CUIMenuWidgets::the().add_slider("Size", "%0.0f", &crosshair_size, "vanilla");
					CUIMenuWidgets::the().add_slider("Gap", "%0.0f", &crosshair_gap, "vanilla");
					CUIMenuWidgets::the().add_slider("Thickness", "%0.0f pixels", &crosshair_thickness);

					CUIMenuWidgets::the().add_listbox("Type ##hidden", &crosshair_type, { "Classic", "T-Shaped", "Circular" });

					CUIMenuWidgets::the().add_color_edit("Color", &crosshair_color);
				});
			});


		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Other", CMenuStyle::calc_child_size(140), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Better cl_showfps", &ingamescreen_better_cl_showfps);
				CUIMenuWidgets::the().add_checkbox("Background", &ingamescreen_better_cl_showfps_background);
				CUIMenuWidgets::the().add_listbox("Position", &ingamescreen_better_cl_showfps_position, { "Top left", "Top right", "bottom right", "bottom left" });
			});

		g_gui_widgets_i->end_columns();
	}
}

void CUIMenu::tab_visuals4()
{
	g_gui_widgets_i->add_text(__FUNCTION__ "()");
}

void CUIMenu::tab_exploits()
{
	CUIMenuWidgets::the().add_menu_child_collapsible(
		"Server command filter", CMenuStyle::child_full_width(210.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			if (g_gui_widgets_i->begin_columns("server_cmd_filter", 3))
			{
				g_gui_widgets_i->setup_column_fixed_width(100);
				g_gui_widgets_i->setup_column_fixed_width(150);

				g_gui_widgets_i->goto_next_column();

				CUIMenuWidgets::the().add_checkbox("Enable", &cmdfilter_enable);
				CUIMenuWidgets::the().add_checkbox("Filter all", &cmdfilter_filter_all);

				g_gui_widgets_i->goto_next_column();

				CUIMenuWidgets::the().add_checkbox("Print blocked cmds", &cmdfilter_print_blocked);

				g_gui_widgets_i->goto_next_column();

				CUIMenuWidgets::the().add_checkbox("Print every cmds", &cmdfilter_print_every);

				g_gui_widgets_i->end_columns();
			}

			CUIMenuWidgets::the().add_description_text_ex(
				"This filter allows you to block commands that are send to you from the server."
				" You can add a list of commands separated by a comma \";\" that you want to block, such as:\n"
				"\"fps_max; bind; exit\" and such.",

				[&]()
				{
					g_gui_widgets_i->add_text(
						"How does it work", 
						TEXTPROP_Wrapped,
						g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText, FDC_Regular));

					g_gui_widgets_i->add_text(
						"For example, often the server sends command to you such as \"fps_max 100; developer 0\" etc."
						" Therefore, you can add the command that you don't want to have executed by the server here."
						" In theory, this is the same as cl_filterstuffcmd however, more customizable.", 
						TEXTPROP_Wrapped);
				}
			);

			g_gui_widgets_i->add_table(
				"cmdfilter_table", 2,
				ImGuiTableFlags_HeaderTextOnly,
				[&]()
				{
					static auto column_flags = ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoResize;
					g_gui_widgets_i->table_setup_column_fixed_width("Commands to be filtered", 350.0f, column_flags);
				},
				[&]()
				{
					g_gui_widgets_i->table_next_column();

					static char cmd_buffer[1024];
					static bool at_init = false;
					if (!at_init)
					{
						strcpy_s(cmd_buffer, cmdfilter_filtered_commands.get_value_string());
						at_init = true;
					}

					bool reclaim_focus_key = false;
					if (g_gui_widgets_i->add_text_input_ex("Commands to be filtered", cmd_buffer, sizeof(cmd_buffer),
														   Vector2D(-1.0f, 0.0f)))
					{
						reclaim_focus_key = true;
					};

					// Auto-focus on window apparition
					g_gui_widgets_i->set_item_default_focus();
					if (reclaim_focus_key)
					{
						g_gui_widgets_i->set_keyboard_focus_here(-1); // Auto focus previous widget
					}

					g_gui_widgets_i->table_next_column();

					if (g_gui_widgets_i->add_button("Apply", { -1.0f, 25.0f }, false, BUTTONFLAG_CenterLabel))
					{
						if (cmd_buffer[0])
						{
							cmdfilter_filtered_commands.set_value(cmd_buffer);
						}
					}
				});
		});

	if (g_gui_widgets_i->begin_columns(__FUNCTION__, 2))
	{
		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Bypass game constrains", CMenuStyle::calc_child_size(325), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Re-enable noclip", &bypass_constrain_noclip, "Makes it possible to re-enable noclip in singleplayer. sv_cheats must be enabled.");

				CUIMenuWidgets::the().add_checkbox("Enable", &bypass_constrain_renderdist_enable);
				CUIMenuWidgets::the().add_slider("Render distance", "%0.0f units", &bypass_constrain_renderdist_value);

				CUIMenuWidgets::the().add_checkbox("Re-enable renderer cvars", &bypass_constrain_renderer_cvars);

				CUIMenuWidgets::the().add_description_text(
					"Disables R_ForceCVars, which is responsible for preventing some renderer cvars to be set.",

					"These cvars are:\n\nr_lightmap, gl_clear, r_novis, r_fullbright, snd_show, chase_active, gl_monolights, gl_wireframe, r_dynamic, gl_alphamin, gl_max_size, gl_polyoffset, r_drawentities.");


				CUIMenuWidgets::the().add_checkbox("Disable SP-Only cvars", &bypass_constrain_sponly_cvars,
												   "Some cvars can be only set in singleplayer. Set this to be able to control these cvars also in MP. "
												   "See the console output for which cvars this affects.");
			});

		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Frame skip", CMenuStyle::calc_child_size(175), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&frame_skip_enable,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Amount", "%0.0f frames", &frame_skip_amount);

					CUIMenuWidgets::the().add_description_text(
						"Frame skip or \"Frame simulation\" enables \"fake\" fps, that are not visual, but physical.",

						"What it does is that it allows execution of the main rendering code only every Nth frame (based on the settings).\n"
						"This allows for massive fps boosts, since basically everything is being rendered each Nth frame only.\n"
						"If you set this up to maximum value, you will get maximum fps boost, but bigger visual lags. On the contrary, you will get less fps boost, but without visual lags.\n\n"
						"Note that above 1000fps the engine isn't really functioning as it should.");

					CUIMenuWidgets::the().add_slider("FPS limit", "~%0.0f frames/sec", &frame_skip_maxfps);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Consistency bypass", CMenuStyle::calc_child_size(150), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&consistencybypass_enable,
				[]()
				{
					CUIMenuWidgets::the().add_checkbox("Logging", &consistencybypass_log);

					CUIMenuWidgets::the().add_description_text(
						"This enables to send false consistency information to the server, when it gets requested.",

						"Consistency in GoldSrc games is a technique that enables servers to check whenever clients have same files"
						" as the server does. The server can also check for malicious DLL files anywhere inside your Half-Life folder (cheat dlls)."
						" Plugins use this to detect cheats when you're connecting to a server and can ban you if they find some.\n\n"
						"This allows you to bypass not only the checks for invalid dlls, but also for other things such as models, sprites, sounds, etc.");
				});
			});

		g_gui_widgets_i->end_columns();
	}

	CUIMenuWidgets::the().add_menu_child_collapsible(
		"Lie to the server", CMenuStyle::child_full_width(400.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[this]()
		{
			if (g_gui_widgets_i->begin_columns("server_liar_columns", 2))
			{
				g_gui_widgets_i->setup_column_fixed_width(200);
				g_gui_widgets_i->goto_next_column();

				CUIMenuWidgets::the().add_checkbox("Enable", &cvarfilter_enable);

				g_gui_widgets_i->add_spacing();

				CUIMenuWidgets::the().add_checkbox("Monitor server", &cvarfilter_monitor_server,
												   "This enables to see what cvars is the server requesting."
												   " See the console after you join a server, to see what cvars it requests from you.");

				g_gui_widgets_i->goto_next_column();

				g_gui_widgets_i->add_text("Description");

				CUIMenuWidgets::the().add_description_text(
					"The \"server lying\" technique is used to \"fake\" the server with cvar values.",

					"Occasionally, some servers request cvar values from the client such as \"fps_max\" etc. and"
					" they basically want to know the value - in order to kick you when the value is \"against the rules\"."
					"\n\nUsing this technique, you can basically send whatever cvar values you want to the server, when it requests it."
					"\n\nFor example, the server wants you to have \"fps_max\" set to \"100\", but you don't want to. Change the value here, "
					"and when the server will request this cvar value, it will send him whatever value you've specified here, instead of the real one."
				);

				g_gui_widgets_i->end_columns();
			}

			// too much code to have inside of the menu...
			CServerLiar::the().render_ui();
		});
}

void CUIMenu::tab_movement()
{
	if (g_gui_widgets_i->begin_columns(__FUNCTION__, 2))
	{
		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Air stuck", CMenuStyle::calc_child_size(90), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section_incommands(
				&CMovement::the().airstuck,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Intensity", "%0.0f", &movement_air_stuck_intensity, "froze");
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Visualization", CMenuStyle::calc_child_size(170), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				g_gui_widgets_i->add_separtor_with_text("Graph plotter");

				CUIMenuWidgets::the().feature_enabled_section(
				&movement_plot,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Scale", "%0.0fx", &movement_plot_scale);
					CUIMenuWidgets::the().add_slider("Row height", "%0.0fpx", &movement_plot_row_height);

					g_gui_widgets_i->add_spacing();
					CUIMenuWidgets::the().add_checkbox("Stop collection", &movement_plot_stop);
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Ground strafe", CMenuStyle::calc_child_size(300), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section_incommands(
				&CMovement::the().gs,
				[]()
				{
					g_gui_widgets_i->add_separtor_with_text("No slowdown");

					CUIMenuWidgets::the().feature_enabled_section(
					&movement_gs_mode_noslowdown,
					[]()
					{
						CUIMenuWidgets::the().add_listbox("Method##nsdn", &movement_gs_mode_noslowdown_method, { "Server speedhack", "Engine speedhack" });

						g_gui_widgets_i->add_spacing();

						CUIMenuWidgets::the().add_slider("Speed factor", "%0.0fx", &movement_gs_mode_noslowdown_factor, NULL, "full speed");
					}, "Enable##nsdn");

					g_gui_widgets_i->add_separtor_with_text("Method");
					CUIMenuWidgets::the().add_listbox("Mode", &movement_gs_mode, { "Legit", "Rage" });

					g_gui_widgets_i->add_spacing();
					g_gui_widgets_i->add_separator();

					if (movement_gs_mode.get_value() == 0) // legit
					{
						g_gui_widgets_i->add_separtor_with_text("Scroll emulation");
						g_gui_widgets_i->add_text("Ground distance to start scrolling");
						CUIMenuWidgets::the().add_description_text("Represents min&max distance from the ground where the randomized scrolling will begin. Same as in bunnyhop.");
						g_gui_widgets_i->add_spacing();
						CUIMenuWidgets::the().add_slider_nolabel("min", "%0.0f units", &movement_gs_legit_ground_dist_min);
						CUIMenuWidgets::the().add_slider_nolabel("max", "%0.0f units", &movement_gs_legit_ground_dist_max);

						int min = movement_gs_legit_ground_dist_min.get_value();
						int max = movement_gs_legit_ground_dist_max.get_value();

						// auto-correct nonsense values
						if (min > max)
						{
							movement_gs_legit_ground_dist_min.set_value(max);
						}
						if (max < min)
						{
							movement_gs_legit_ground_dist_max.set_value(min);
						}

						CUIMenuWidgets::the().add_slider("Scroll density", "%0.0f", &movement_gs_legit_scroll_density, NULL, "full");

						CUIMenuWidgets::the().add_listbox("Efficiency", &movement_gs_legit_efficiency, { "Helper", "Normal", "Random FOG based" });

						CUIMenuWidgets::the().add_description_text("Same settings as in bunnyhop. I don't want to repeat myself here ;)");
					}
					else if (movement_gs_mode.get_value() == 1) // rage
					{
						CUIMenuWidgets::the().add_description_text(
							"Rage bhop is highly obviously to someone, who analyzes your demo - be careful where you use it. "
							"\n\nHowever, it suits well for HVH scenarious or simply for servers where no one cares about bhop hacks. "
							"It also has no-slow-down capabilities.");
					}
				});
			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
		"Strafe hack", CMenuStyle::calc_child_size(225), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			CUIMenuWidgets::the().feature_enabled_section_incommands(
			&CMovement::the().strafe,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Allow on surf", &movement_strafe_hack_allow_on_surf);
				CUIMenuWidgets::the().add_checkbox("Reset to forward", &movement_strafe_hack_dir_reset);

				CUIMenuWidgets::the().feature_enabled_section(
				&movement_strafe_hack_dir_auto,
				[]()
				{
					CUIMenuWidgets::the().add_listbox("Direction", &movement_strafe_hack_dir, { "Forward", "Right", "Back", "Left" });
				}, 
				"Automatic Direction", 
				false);

				g_gui_widgets_i->add_spacing();
				CUIMenuWidgets::the().add_slider("Boost", "%0.2f", &movement_strafe_hack_boost);

				CUIMenuWidgets::the().feature_enabled_section(
				&movement_strafe_hack_limit_velocity,
				[]()
				{
					CUIMenuWidgets::the().add_slider_nolabel("Max", "%0.0f units", &movement_strafe_hack_limit_velocity_max);
				},
				"Limit velocity");
			});
		});

		CUIMenuWidgets::the().add_menu_child_collapsible(
		"Strafe helper", CMenuStyle::calc_child_size(240), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			CUIMenuWidgets::the().feature_enabled_section_incommands(
			&CMovement::the().strafe_helper,
			[]()
			{
				CUIMenuWidgets::the().add_slider("Accumulation", "%0.2f", &movement_strafe_helper_accumulation);//
				CUIMenuWidgets::the().add_checkbox("Accumulation on ground", &movement_strafe_helper_accumulation_on_ground);

				CUIMenuWidgets::the().feature_enabled_section(
				&movement_strafe_helper_strafe_with_mouse,
				[]()
				{
					CUIMenuWidgets::the().add_listbox("Direction", &movement_strafe_helper_strafe_dir, { "Forward", "Right", "Back", "Left" });
					CUIMenuWidgets::the().add_checkbox("Deduce automatically", &movement_strafe_helper_strafe_dir_auto);
				}, "Strafe with mouse");
			});
		});

		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
		"Bunny hop", CMenuStyle::calc_child_size(300), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			CUIMenuWidgets::the().feature_enabled_section_incommands(
			&CMovement::the().bunnyhop,
			[]()
			{
				g_gui_widgets_i->add_separtor_with_text("No slowdown");
				CUIMenuWidgets::the().feature_enabled_section(
				&movement_bhop_mode_noslowdown,
				[]()
				{
					CUIMenuWidgets::the().add_listbox("Method##nsdn", &movement_bhop_mode_noslowdown_method, { "Server speedhack", "Engine speedhack" });

					g_gui_widgets_i->add_spacing();
					CUIMenuWidgets::the().add_slider("Speed factor", "%0.0fx", &movement_bhop_mode_noslowdown_factor, NULL, "full speed");

					g_gui_widgets_i->add_spacing();

					CUIMenuWidgets::the().add_description_text("Note that this uses speedhack, which most of the anti-cheats can detect.");
				}, "Enable##nsdn");

				g_gui_widgets_i->add_separtor_with_text("Others");
				g_gui_widgets_i->add_spacing();
				CUIMenuWidgets::the().add_slider("Jump repeat", "%0.0f ms", &movement_bhop_repeat_ms, "no limit");

				CUIMenuWidgets::the().add_checkbox("Jump if on ladder", &movement_bhop_jump_on_ladder);
				CUIMenuWidgets::the().add_checkbox("Jump if in water", &movement_bhop_jump_in_water);

				g_gui_widgets_i->add_separtor_with_text("Method");
				CUIMenuWidgets::the().add_listbox("Mode##nolabel", &movement_bhop_mode, { "Legit", "Rage" });

				g_gui_widgets_i->add_spacing();
				g_gui_widgets_i->add_separator();

				if (movement_bhop_mode.get_value() == 0) // legit
				{
					g_gui_widgets_i->add_separtor_with_text("Scroll emulation");
					g_gui_widgets_i->add_text("Ground distance to start scrolling");
					CUIMenuWidgets::the().add_description_text("Represents min&max distance from the ground where the randomized scrolling will begin.");
					g_gui_widgets_i->add_spacing();
					CUIMenuWidgets::the().add_slider_nolabel("min", "%0.0f units", &movement_bhop_legit_ground_dist_min);
					CUIMenuWidgets::the().add_slider_nolabel("max", "%0.0f units", &movement_bhop_legit_ground_dist_max);

					int min = movement_bhop_legit_ground_dist_min.get_value();
					int max = movement_bhop_legit_ground_dist_max.get_value();

					// auto-correct nonsense values
					if (min > max)
					{
						movement_bhop_legit_ground_dist_min.set_value(max);
					}
					if (max < min)
					{
						movement_bhop_legit_ground_dist_max.set_value(min);
					}

					CUIMenuWidgets::the().add_slider("Scroll density", "%0.0f", &movement_bhop_legit_scroll_density, NULL, "full");

					CUIMenuWidgets::the().add_listbox("Efficiency", &movement_bhop_legit_efficiency, { "Helper", "Normal", "Random FOG based" });

					CUIMenuWidgets::the().add_description_text("Helper tries to predict whenever you'll be on ground next frame - therefore can execute a perfect jump on that next frame.");
				}
				else if (movement_bhop_mode.get_value() == 1) // rage
				{
					CUIMenuWidgets::the().add_description_text(
						"Rage bhop is highly obviously to someone, who analyzes your demo - be careful where you use it. "
						"\n\nHowever, it suits well for HVH scenarious or simply for servers where no one cares about bhop hacks. "
						"It also has no-slow-down capabilities.");
				}
			});
		});

		CUIMenuWidgets::the().add_menu_child_collapsible(
		"Edge bug", CMenuStyle::calc_child_size(290), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			CUIMenuWidgets::the().feature_enabled_section_incommands(
			&CMovement::the().eb,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Enable on ramps", &movement_eb_enable_on_ramps);

				g_gui_widgets_i->add_separtor_with_text("Tweaking");

				CUIMenuWidgets::the().add_description_text("Each of these specified the scenario where the edgebug can happen.");

				g_gui_widgets_i->add_spacing();
				CUIMenuWidgets::the().add_slider("Intensity", "%0.1fx", &movement_eb_intensity);
				CUIMenuWidgets::the().add_slider("Fall velocity", "%0.0f units", &movement_eb_min_fall_velocity);
				CUIMenuWidgets::the().add_slider("Edge dist", "%0.0f units", &movement_eb_edge_dist);
				CUIMenuWidgets::the().add_slider("Ground dist", "%0.0f units", &movement_eb_ground_dist);
			});
		});

		CUIMenuWidgets::the().add_menu_child_collapsible(
		"Fast run", CMenuStyle::calc_child_size(200), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			CUIMenuWidgets::the().feature_enabled_section_incommands(
			&CMovement::the().fastrun,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("No slowdown", &movement_fastrun_no_slowdown);
				CUIMenuWidgets::the().add_slider("Max speed##0", "%0.0f u/s", &movement_fastrun_max_speed);

				g_gui_widgets_i->add_separtor_with_text("Fast walk");
				CUIMenuWidgets::the().add_checkbox("Enable", &movement_fastrun_fast_walk);
				CUIMenuWidgets::the().add_slider("Max speed##1", "%0.0f u/s", &movement_fastrun_fast_walk_max_speed);
			});
		});

		CUIMenuWidgets::the().add_menu_child_collapsible(
		"Auto JOF", CMenuStyle::calc_child_size(200), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			CUIMenuWidgets::the().feature_enabled_section_incommands(
			&CMovement::the().auto_jof,
			[]()
			{
				CUIMenuWidgets::the().add_slider("Interval", "%0.0f ms", &movement_auto_jof_interval);

				g_gui_widgets_i->add_separtor_with_text("Min distance & velocity");
				CUIMenuWidgets::the().feature_enabled_section(
				&movement_auto_jof_auto,
				[]()
				{
					CUIMenuWidgets::the().add_slider("Distance", "%0.1f units", &movement_auto_jof_min_distance);
				}, 
				"Automatic", FALSE);

				CUIMenuWidgets::the().add_slider("Min velocity", "%0.0f u/s", &movement_auto_jof_activation_vel_min, "off");
			});
		});

		g_gui_widgets_i->end_columns();
	}
}

void CUIMenu::tab_miscellaneous2()
{
	g_gui_widgets_i->add_text(__FUNCTION__ "()");
}

void CUIMenu::tab_miscellaneous3()
{
	g_gui_widgets_i->add_text(__FUNCTION__ "()");
}

void CUIMenu::tab_config()
{
	CUIMenuWidgets::the().add_menu_child(
		"Configuration", CMenuStyle::child_full_width(-1.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			// first = error or not, second = status message
			static std::pair<bool, std::string> current_status = {};
			static uint32_t status_tm = GetTickCount();
			auto status_msg = [](const std::string& msg, bool error)
			{
				current_status = std::make_pair(error, msg);
				status_tm = GetTickCount();
			};

			static std::vector<FilePath_t> configs;
			static int num_configs = 0;
			auto search_for_configs = []()
			{
				configs.clear();

				g_config_mgr_i->for_each_cfg(
					[](const FilePath_t& cfg_path)
					{
						num_configs++;

						auto relative_cfg_path = g_filesystem_i->get_relative_to_appdata_ex("config\\", cfg_path);

						configs.push_back(relative_cfg_path);
					});
			};

			static uint32_t last_searched = GetTickCount();

			// search each # amount of ms
			static constexpr uint32_t interval_ms = 250;
			if (GetTickCount() - last_searched > interval_ms)
			{
				search_for_configs();
				last_searched = GetTickCount();
			}

			static FilePath_t selected_cfg = {};
			if (configs.empty() && !selected_cfg.empty())
			{
				selected_cfg.clear();
			}

			float status_bar_footer_height = 33.0f;

			if (g_gui_widgets_i->begin_columns(__FUNCTION__, 2))
			{
				g_gui_widgets_i->goto_next_column();

				g_gui_widgets_i->add_text("Configuration files");

				g_gui_widgets_i->push_stylevar(ImGuiStyleVar_WindowPadding, Vector2D(5, 5));

				g_gui_widgets_i->add_child(
					"configs", { -1.0f, -1.0f - status_bar_footer_height }, true, ImGuiWindowFlags_AlwaysUseWindowPadding,
					[]()
					{
						if (configs.empty())
						{
							g_gui_widgets_i->add_window_centered_text_disabled("No configuration files available");
						}
						else
						{
							for (const auto& cfg_path : configs)
							{
								bool is_selected = selected_cfg == cfg_path;
								if (g_gui_widgets_i->add_toggle_button(cfg_path.string(), { -1.0f, 0.0f },
																	   is_selected, false))
								{
									if (is_selected)
									{
										// clicking on currently selected entry, toggle it.
										selected_cfg.clear();
									}
									else
									{
										selected_cfg = cfg_path;
									}
								}
							}
						}
					});

				g_gui_widgets_i->goto_next_column();

				g_gui_widgets_i->add_text("Options on selected cfg");

				g_gui_widgets_i->add_child(
					"selection", { -1.0f, 55 }, true, ImGuiWindowFlags_AlwaysUseWindowPadding,
					[&]()
					{
						if (!selected_cfg.empty())
						{
							if (g_gui_widgets_i->add_button("load", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
							{
								if (g_config_mgr_i->load_configuration(CFG_CheatSettings, selected_cfg.string()))
								{
									status_msg(std::format("Loaded from {}.", selected_cfg.string()), false);
								}
								else
								{
									status_msg(std::format("Failed to load {}!", selected_cfg.string()), true);
								}
							}

							if (g_gui_widgets_i->add_button("delete", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
							{
								if (g_filesystem_i->remove(g_appdata_mgr_i->get_known("config\\") / selected_cfg))
								{
									status_msg(std::format("Deleted {}.", selected_cfg), false);
								}
								else
								{
									status_msg(std::format("Failed to delete {}.", selected_cfg), true);
								}
							}
						}
						else
						{
							g_gui_widgets_i->add_window_centered_text_disabled("No cfg selected");
						}
					});

				g_gui_widgets_i->add_text("Other");

				g_gui_widgets_i->add_child(
					"information", { -1.0f, -1.0f - status_bar_footer_height }, true, ImGuiWindowFlags_AlwaysUseWindowPadding,
					[&]()
					{
						if (g_gui_widgets_i->add_button("create new", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
						{
							static char name_buffer[64];
							COxWareUI::the().schedule_popup(
								"", Vector2D(210, 170),
								[]()
								{
									if (g_gui_widgets_i->add_text_input("Config name", name_buffer, sizeof(name_buffer), ImGuiInputTextFlags_EnterReturnsTrue, true))
									{
										// enter returns true:
										COxWareUI::the().close_current_popup();
									}

									g_gui_widgets_i->add_text("Enter name of the config");

									g_gui_widgets_i->add_text("Will be saved as:");

									if (name_buffer[0])
									{
										g_gui_widgets_i->add_text(std::format("'{}.json'", name_buffer),
																  TEXTPROP_Wrapped, g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.medium(), FDC_Regular));
									}
								},
								[&]() // on close
								{
									if (!name_buffer[0])
									{
										return;
									}

									auto path = g_config_mgr_i->get_config_directory(name_buffer);

									if (!path.has_extension() || path.extension() != ".json")
									{
										path.replace_extension("json");
									}

									if (g_config_mgr_i->write_configuration(CFG_CheatSettings, path.filename().string()))
									{
										status_msg(std::format("Saved '{}'.", path.filename().string()), false);
									}
									else
									{
										status_msg(std::format("Couldn't save '{}'.", path.filename().string()), true);
									}

									strcpy(name_buffer, "");
								}, 
								ImGuiWindowFlags_NoResize);
						}

						if (g_gui_widgets_i->add_button("open config directory", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
						{
							auto dir = g_config_mgr_i->get_config_directory().string();
							CGenericUtil::the().open_folder_inside_explorer(dir);
							status_msg("Opened config directory.", false);
						}

						if (g_gui_widgets_i->add_button("save current", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
						{
							if (g_config_mgr_i->write_configuration(CFG_CheatSettings, "saved.json"))
							{
								status_msg("Saved to saved.json.", false);
							}
							else
							{
								status_msg("Failed to save to saved.json!", false);
							}
						}

						if (g_gui_widgets_i->add_button("restore defaults", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
						{
							// this if-else tree is kinda dumb, but whatever xd
							if (g_config_mgr_i->load_configuration(CFG_CheatSettings, "default.json"))
							{
								status_msg("Restored default configuration.", false);
							}
							else
							{
								if (!g_config_mgr_i->write_configuration(CFG_CheatSettings, "default.json"))
								{
									status_msg("Failed to restore defaults!", true);
								}
								else
								{
									if (g_config_mgr_i->load_configuration(CFG_CheatSettings, "default.json"))
									{
										status_msg("Restored default configuration.", false);
									}
									else
									{
										status_msg("Failed to restore defaults!", true);
									}
								}

							}
						}

						g_gui_widgets_i->pop_stylevar();

						g_gui_widgets_i->add_spacing();
						CUIMenuWidgets::the().add_slider("Autosave interval", "%0.0f seconds", g_config_mgr_i->get_save_cfg_interval_var());

						g_gui_widgets_i->add_progress_bar("Test", { -1.0f, 0.0f }, g_config_mgr_i->get_duration_last_saved_sec(), (float)g_config_mgr_i->get_save_cfg_interval_var()->get_value());
					});

				g_gui_widgets_i->end_columns();
			}
				
			g_gui_widgets_i->add_child(
				"status", { -1.0f, -1.0f }, true, ImGuiWindowFlags_AlwaysUseWindowPadding,
				[&]()
				{
					uint32_t dur_sec = (GetTickCount() - status_tm) * 1000;
					auto [error, msg] = current_status;
					if (!msg.empty() && dur_sec < 3)
					{
						auto color = error ? CColor(112, 0, 0, 170) : CColor(0, 112, 0, 170);

						g_gui_widgets_i->push_font(g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.medium(), FDC_Regular));
						g_gui_widgets_i->add_colored_text(color, msg);
						g_gui_widgets_i->pop_font();
					}
				});
		});
}

void CUIMenu::tab_binds()
{
	CUIKeyBinding::the().render_interactible_bind_list();
}

void CUIMenu::tab_incommands()
{
	CUIInCommandKeyBinding::the().render_interactible_list();
}

void CUIMenu::tab_cmdlist()
{
	CUIMenuWidgets::the().add_menu_child(
		"Command list", CMenuStyle::child_full_width(-1.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			static char buffer[256];
			static bool once = false;
			if (!once)
			{
				strcpy_s(buffer, "");
				once = true;
			}

			if (g_gui_widgets_i->add_text_input_ex("Search for a variable", buffer, sizeof(buffer),
												   Vector2D(-1.0f, 0.0f)))
			{
			};

			bool should_filter = buffer[0];

			g_gui_widgets_i->add_table(
				"command_list", 4, 
				ImGuiTableFlags_HeaderTextOnly | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
				[]()
				{
					static auto flags = ImGuiTableColumnFlags_NoReorder;

					g_gui_widgets_i->table_setup_column_fixed_width("N", 25.0f, flags);
					g_gui_widgets_i->table_setup_column("Name", flags);
					g_gui_widgets_i->table_setup_column("Usage", flags);
					g_gui_widgets_i->table_setup_column("Description", flags);
				}, 
				[&]()
				{
					auto small_font = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.small(), FDC_Regular);
					g_gui_widgets_i->push_font(small_font);

					int n = 0;
					g_variablemgr_i->for_each_command(
						[&](BaseCommand* cmd)
						{
							if (should_filter && !strstr(cmd->get_name(), buffer))
							{
								return;
							}
							
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(std::format("{}", ++n));
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(cmd->get_name());
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(cmd->has_usage() ? cmd->get_usage() : "");
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(cmd->get_description());
						});

					g_gui_widgets_i->pop_font();

				});
		});
}

void CUIMenu::tab_varlist()
{
	CUIMenuWidgets::the().add_menu_child(
		"Variable list", CMenuStyle::child_full_width(-1.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
		[]()
		{
			static char buffer[256];
			static bool once = false;
			if (!once)
			{
				strcpy_s(buffer, "");
				once = true;
			}

			if (g_gui_widgets_i->add_text_input_ex("Search for a variable", buffer, sizeof(buffer),
												   Vector2D(-1.0f, 0.0f)))
			{
			};

			bool should_filter = buffer[0];

			g_gui_widgets_i->add_table(
				"variable_list", 7,
				ImGuiTableFlags_HeaderTextOnly | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
				[]()
				{
					static auto flags = ImGuiTableColumnFlags_NoReorder;

					g_gui_widgets_i->table_setup_column_fixed_width("N", 25.0f, flags);
					g_gui_widgets_i->table_setup_column("Name", flags);
					g_gui_widgets_i->table_setup_column_fixed_width("Value", 60.0f, flags);
					g_gui_widgets_i->table_setup_column_fixed_width("Default", 60.0f, flags);
					g_gui_widgets_i->table_setup_column_fixed_width("Min", 60.0f, flags);
					g_gui_widgets_i->table_setup_column_fixed_width("Max", 60.0f, flags);
					g_gui_widgets_i->table_setup_column("Description", flags);
				},
				[&]()
				{
					auto small_font = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.small(), FDC_Regular);
					g_gui_widgets_i->push_font(small_font);

					int n = 0;
					g_variablemgr_i->for_each_variable(
						[&](BaseVariable* var)
						{
							if (should_filter && !strstr(var->get_name(), buffer))
							{
								return;
							}

							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(std::format("{}", ++n));
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(var->get_name());
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(var->get_value_string());
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(var->get_default_value_string());
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(var->get_min_string());
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(var->get_max_string());
							g_gui_widgets_i->table_next_column();
							g_gui_widgets_i->add_text(var->get_description());
						});

					g_gui_widgets_i->pop_font();

				});
		});
}

void CUIMenu::tab_others()
{
	if (g_gui_widgets_i->begin_columns(__FUNCTION__, 2))
	{
		g_gui_widgets_i->goto_next_column();

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"UI", CMenuStyle::calc_child_size(180), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().add_checkbox("Enable rain", &ui_background_rain);
				CUIMenuWidgets::the().add_checkbox("Enable background fade", &ui_background_fade);

				g_gui_widgets_i->add_separtor_with_text("Menu");
				CUIMenuWidgets::the().add_checkbox("Resizable (beta)", &ui_menu_resizable);

				g_gui_widgets_i->add_separtor_with_text("Feature list");
				CUIMenuWidgets::the().feature_enabled_section(
				&ui_render_feature_list,
				[]()
				{
					CUIMenuWidgets::the().add_listbox("Font type", &ui_feature_list_font, { "Bold", "Regular" });
					CUIMenuWidgets::the().add_listbox("Font size", &ui_feature_list_font_size, { "Smaller", "Bigger" });

					CUIMenuWidgets::the().add_checkbox("Check for overlow", &ui_feature_list_overflow);
				}, "Show");

				g_gui_widgets_i->add_separtor_with_text("Fonts");

				static bool no_hinting = g_gui_fontmgr_i->get_freetype_builder_flags() & FreeTypeBuilderFlags_NoHinting;
				static bool no_auto_hint = g_gui_fontmgr_i->get_freetype_builder_flags() & FreeTypeBuilderFlags_NoAutoHint;
				static bool force_auto_hint = g_gui_fontmgr_i->get_freetype_builder_flags() & FreeTypeBuilderFlags_ForceAutoHint;
				static bool light_hinting = g_gui_fontmgr_i->get_freetype_builder_flags() & FreeTypeBuilderFlags_LightHinting;
				static bool mono_hinting = g_gui_fontmgr_i->get_freetype_builder_flags() & FreeTypeBuilderFlags_MonoHinting;

				if (g_gui_widgets_i->add_checkbox("No hinting", &no_hinting)) g_gui_fontmgr_i->add_freetype_builder_flags(FreeTypeBuilderFlags_NoHinting, no_hinting);
				if (g_gui_widgets_i->add_checkbox("No auto-hint", &no_auto_hint)) g_gui_fontmgr_i->add_freetype_builder_flags(FreeTypeBuilderFlags_NoAutoHint, no_auto_hint);
				if (g_gui_widgets_i->add_checkbox("Force auto-hint", &force_auto_hint)) g_gui_fontmgr_i->add_freetype_builder_flags(FreeTypeBuilderFlags_ForceAutoHint, force_auto_hint);
				if (g_gui_widgets_i->add_checkbox("Light hinting", &light_hinting)) g_gui_fontmgr_i->add_freetype_builder_flags(FreeTypeBuilderFlags_LightHinting, light_hinting);
				if (g_gui_widgets_i->add_checkbox("Mono hinting", &mono_hinting)) g_gui_fontmgr_i->add_freetype_builder_flags(FreeTypeBuilderFlags_MonoHinting, mono_hinting);


			});

		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Debug", CMenuStyle::calc_child_size(300), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				CUIMenuWidgets::the().feature_enabled_section(
				&debug,
				[]()
				{
					CUIMenuWidgets::the().add_description_text("This feature is used for developement only. You have been warned.");

					g_gui_widgets_i->begin_tab("debug_tab", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll);

					float tab_height = -1.0f;

					g_gui_widgets_i->add_tab_item(
						"Rendering", false,
						{ -1.0f, tab_height },
						[]()
						{
							CUIMenuWidgets::the().feature_enabled_section(
							&debug_render_info,
							[]()
							{
								g_gui_widgets_i->add_spacing();
								g_gui_widgets_i->add_separtor_with_text("Movement");
								CUIMenuWidgets::the().feature_enabled_section(
								&debug_render_info_movement,
								[]()
								{
									g_gui_widgets_i->add_spacing();
									CUIMenuWidgets::the().add_checkbox("Bhop", &debug_render_info_movement_bhop);
									CUIMenuWidgets::the().add_checkbox("Strafe hack", &debug_render_info_movement_strafe);
									CUIMenuWidgets::the().add_checkbox("Strafe helper", &debug_render_info_movement_strafe_helper);
								}, "Enable##movement");
							});
						});

					g_gui_widgets_i->end_tab();
				});
			});

		g_gui_widgets_i->goto_next_column();
		
		CUIMenuWidgets::the().add_menu_child_collapsible(
			"Storage", CMenuStyle::calc_child_size(200), false, ImGuiWindowFlags_AlwaysUseWindowPadding,
			[]()
			{
				auto font = g_gui_fontmgr_i->get_font(FID_SegoeUI, FontSize::UIText.medium(), FDC_Regular);

				auto cheat_directory = g_filesystem_i->get_appdata_path();

				static uintmax_t cheat_directory_size = 0, logs_directory_size = 0, cfg_directory_size = 0;

				static uint32_t t1 = GetTickCount();

				if (GetTickCount() - t1 > 250)
				{
					cheat_directory_size = g_filesystem_i->directory_size(cheat_directory);
					logs_directory_size = g_filesystem_i->directory_size(cheat_directory / "log");
					cfg_directory_size = g_filesystem_i->directory_size(cheat_directory / "config");

					t1 = GetTickCount();
				}

				g_gui_widgets_i->add_text(std::format("Currently occupied space on your computer: {}", 
													  CStringTools::the().pretify_file_size((float)cheat_directory_size)), TEXTPROP_Wrapped, font);

				g_gui_widgets_i->add_separator();
				
				g_gui_widgets_i->add_text(std::format("logs: {}",
													  CStringTools::the().pretify_file_size((float)logs_directory_size)), TEXTPROP_Wrapped, font);
				g_gui_widgets_i->add_text(std::format("configs: {}", 
													  CStringTools::the().pretify_file_size((float)cfg_directory_size)), TEXTPROP_Wrapped, font);

				g_gui_widgets_i->add_separator();

				if (g_gui_widgets_i->add_button("open cheat storage directory", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
				{
					CGenericUtil::the().open_folder_inside_explorer(cheat_directory.string());
				}

				if (g_gui_widgets_i->add_button("clear unnecessary files", { -1.0f, 0.0f }, false, BUTTONFLAG_CenterLabel))
				{
					g_filesystem_i->iterate_through_files(
						cheat_directory / "log", false, 
						[](const FilePath_t& dir)
						{
							if (g_filesystem_i->is_file(dir))
							{
								g_filesystem_i->remove(dir);
							}
						});
				}

				VarInteger* var = (VarInteger*)g_variablemgr_i->query_variable("num_logfiles_to_keep");

				if (var)
				{
					g_gui_widgets_i->add_spacing();
					CUIMenuWidgets::the().add_slider("Logfiles to keep", "%0.0f", var);
				}
			});

		g_gui_widgets_i->end_columns();
	}
}
