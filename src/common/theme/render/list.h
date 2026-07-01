#ifndef RENDER_MENU_H__
#define RENDER_MENU_H__

#include "components/list.h"
#include "theme/background.h"
#include "theme/config.h"
#include "theme/resources.h"

// static SDL_Color color_black = {0, 0, 0};

// Marquee scroll settings for the active-item label when the preview panel is visible
#define LIST_LABEL_SCROLL_SPEED  2  // pixels advanced per render frame
#define LIST_LABEL_SCROLL_DELAY 30  // frames of initial pause before scrolling starts
#define LIST_LABEL_SCROLL_PAUSE 20  // frames to hold at end before resetting to start
#define LIST_LABEL_END_MARGIN   30  // right-side padding (px) mirroring theme_renderListLabel

// Persistent state for the active-item marquee scroll.
// Rendering is single-threaded (SDL main loop), so plain statics are safe.
static SDL_Surface *list_scroll_surface = NULL;
static int          list_scroll_x       = 0;  // negative = initial delay phase
static int          list_scroll_list_id = -1;
static int          list_scroll_active  = -1;

void theme_renderListLabel(SDL_Surface *screen, const char *label, SDL_Color fg,
                           int offset_x, int center_y, bool is_active,
                           int label_end, bool disabled)
{
    TTF_Font *list_font = resource_getFont(LIST);

    SDL_Surface *item_label = TTF_RenderUTF8_Blended(list_font, label, fg);
    SDL_Rect item_label_rect = {offset_x, center_y - item_label->h / 2};

    SDL_Rect label_crop = {0, 0, label_end - 30 * g_scale, item_label->h};

    /* Maybe shadows will be an option in the future
    SDL_Rect item_shadow_rect = {item_label_rect.x + 1, item_label_rect.y + 2};

    if (is_active) {
        SDL_Surface *item_shadow =
            TTF_RenderUTF8_Blended(list_font, label, color_black);
        SDL_BlitSurface(item_shadow, &label_crop, screen, &item_shadow_rect);
        SDL_FreeSurface(item_shadow);
    }*/

    if (disabled) {
        surfaceSetAlpha(item_label, HIDDEN_ITEM_ALPHA);
    }

    SDL_BlitSurface(item_label, &label_crop, screen, &item_label_rect);
    SDL_FreeSurface(item_label);
}

typedef struct {
    SDL_Surface *background;
    SDL_Rect dim;
    SDL_Rect pos;
    bool show_dividers;
    bool stretch_y;
    bool preview_bg;
    bool preview_stretch;
    int preview_width;
    bool preview_smoothing;
} ListRenderParams_s;

void theme_renderListCustom(SDL_Surface *screen, List *list, ListRenderParams_s params)
{
    if (params.background)
        SDL_BlitSurface(params.background, &params.dim, screen, &params.pos);

    bool list_small = list->list_type == LIST_SMALL;

    TTF_Font *list_font = resource_getFont(LIST);

    SDL_Rect item_bg_rect = {0, 60.0 * g_scale},
             item_div_size = {0, 0, params.dim.w, 4.0 * g_scale},
             item_bg_size = {0, 0, params.dim.w, (list_small ? 56.0 : 90.0) * g_scale},
             toggle_rect = {0, 0};

    int list_height = params.stretch_y ? params.dim.h - 10.0 * g_scale : 420.0 * g_scale;
    int item_padding = list_small && params.show_dividers ? 4.0 * g_scale : 0;
    int item_height = params.stretch_y ? list_height / list->scroll_height : (list_small ? 60.0 : 90.0) * g_scale;
    int label_y = (list_small ? 27.0 : 37.0) * g_scale;
    SDL_Surface *item_bg = resource_getSurface(list_small ? BG_LIST_S : BG_LIST_L);

    int menu_pos_y = params.stretch_y ? params.pos.y + 4.0 * g_scale + (list_height - list->scroll_height * item_height) / 2
                                      : 420.0 * g_scale - list->scroll_height * item_height;
    int last_item = list->scroll_pos + list->scroll_height;

    if (last_item > list->item_count)
        last_item = list->item_count;

    ListItem *active_preview = NULL;

    // Check if there are hidden items
    bool has_disabled_items = false;
    for (int i = list->scroll_pos; i < last_item; i++) {
        if (list->items[i].disabled && !list->items[i].show_opaque) {
            has_disabled_items = true;
            break;
        }
    }

    // Load hidden items if needed
    if (has_disabled_items) {
        theme_loadHiddenItems();
    }

    if (list->has_sticky) {
        ListItem *active_item = &list->items[list->active_pos];
        if (params.show_dividers)
            SDL_BlitSurface(resource_getSurface(HORIZONTAL_DIVIDER), &item_div_size, screen, &item_bg_rect);
        theme_renderListLabel(screen, active_item->sticky_note, theme()->list.color, 20 * g_scale, item_bg_rect.y + label_y, false, 640 * g_scale, true);
    }

    for (int i = list->scroll_pos; i < last_item; i++) {
        ListItem *item = &list->items[i];
        bool show_disabled = item->disabled && !item->show_opaque;

        item_bg_rect.y = menu_pos_y + (i - list->scroll_pos) * item_height;

        if (params.stretch_y) {
            item_bg_rect.y += (item_height - item_bg_size.h) / 2;
        }

        if (params.show_dividers)
            SDL_BlitSurface(resource_getSurface(HORIZONTAL_DIVIDER), &item_div_size, screen, &item_bg_rect);
        item_bg_rect.y += item_padding;

        if (i == list->active_pos) {
            SDL_BlitSurface(item_bg, &item_bg_size, screen, &item_bg_rect);

            if (item->preview_ptr == NULL && strlen(item->preview_path) > 0 && is_file(item->preview_path)) {
                item->preview_ptr = (void *)IMG_Load(item->preview_path);
            }

            if (item->preview_ptr != NULL)
                active_preview = item;
        }

        const int item_center_y = item_bg_rect.y + item_bg_size.h / 2;
        const int multivalue_width = 226 * g_scale;
        int label_end = 640 * g_scale;
        int offset_x = 20 * g_scale;

        if (item->icon_ptr != NULL) {
            SDL_Surface *icon = (SDL_Surface *)item->icon_ptr;
            if (icon->w > 1) {
                SDL_Rect icon_rect = {offset_x, item_center_y - icon->h / 2};
                SDL_BlitSurface(icon, NULL, screen, &icon_rect);
                offset_x += icon->w + 17 * g_scale;
            }
        }

        if (item->item_type == TOGGLE) {
            SDL_Surface *toggle = show_disabled ? (item->value == 1 ? g_hidden_items.toggle_on : g_hidden_items.toggle_off) : (resource_getSurface(item->value == 1 ? TOGGLE_ON : TOGGLE_OFF));
            toggle_rect.x = 620 * g_scale - toggle->w;
            toggle_rect.y = item_center_y - toggle->h / 2;
            label_end = toggle_rect.x;
            SDL_BlitSurface(toggle, NULL, screen, &toggle_rect);
        }
        else if (item->item_type == MULTIVALUE) {
            SDL_Surface *arrow_left = show_disabled ? g_hidden_items.arrow_left : resource_getSurface(LEFT_ARROW);
            SDL_Surface *arrow_right = show_disabled ? g_hidden_items.arrow_right : resource_getSurface(RIGHT_ARROW);
            SDL_Rect arrow_left_pos = {
                640 * g_scale - 20 * g_scale - arrow_right->w - multivalue_width - arrow_left->w,
                item_center_y - arrow_left->h / 2};
            SDL_Rect arrow_right_pos = {
                640 * g_scale - 20 * g_scale - arrow_right->w,
                item_center_y - arrow_right->h / 2};
            SDL_BlitSurface(arrow_left, NULL, screen, &arrow_left_pos);
            SDL_BlitSurface(arrow_right, NULL, screen, &arrow_right_pos);
            label_end = arrow_left_pos.x;

            char value_str[STR_MAX];
            list_getItemValueLabel(item, value_str);
            SDL_Surface *value_label = TTF_RenderUTF8_Blended(list_font, value_str, theme()->list.color);
            if (show_disabled) {
                surfaceSetAlpha(value_label, HIDDEN_ITEM_ALPHA);
            }
            SDL_Rect value_size = {0, 0, multivalue_width, value_label->h};
            int label_width = value_label->w > value_size.w ? value_size.w : value_label->w;
            SDL_Rect value_pos = {
                640 * g_scale - 20 * g_scale - arrow_right->w - multivalue_width / 2 - label_width / 2,
                item_center_y - value_size.h / 2};
            SDL_BlitSurface(value_label, &value_size, screen, &value_pos);
        }

        // Marquee scroll the active item's label when the preview panel is present
        if (i == list->active_pos && active_preview != NULL) {
            // Reserve space occupied by the right-side preview panel
            SDL_Surface *preview_bg_srf = params.preview_bg ? resource_getSurface(PREVIEW_BG) : NULL;
            int reserved = preview_bg_srf ? preview_bg_srf->w : (int)(params.preview_width * g_scale);
            int preview_label_end = 640 * g_scale - reserved;
            if (preview_label_end < label_end)
                label_end = preview_label_end;

            // Reset scroll state whenever the list or the selected item changes
            if (list->_id != list_scroll_list_id || list->active_pos != list_scroll_active) {
                if (list_scroll_surface != NULL) {
                    SDL_FreeSurface(list_scroll_surface);
                    list_scroll_surface = NULL;
                }
                // Start with a negative counter so the label holds still for
                // LIST_LABEL_SCROLL_DELAY frames before scrolling begins.
                list_scroll_x = -(LIST_LABEL_SCROLL_DELAY * LIST_LABEL_SCROLL_SPEED);
                list_scroll_list_id = list->_id;
                list_scroll_active = list->active_pos;
            }

            // Lazily render the full-width label surface once per selection.
            // If TTF_RenderUTF8_Blended fails, list_scroll_surface stays NULL
            // and theme_renderListLabel is used as a fallback (see below).
            if (list_scroll_surface == NULL)
                list_scroll_surface = TTF_RenderUTF8_Blended(list_font, item->label, theme()->list.color);

            int avail_w = label_end - offset_x - LIST_LABEL_END_MARGIN * g_scale;

            if (list_scroll_surface != NULL && avail_w > 0 && list_scroll_surface->w > avail_w) {
                // Label is wider than the available space: apply marquee scrolling
                int px = list_scroll_x > 0 ? list_scroll_x : 0;
                int max_offset = list_scroll_surface->w - avail_w;
                if (px > max_offset)
                    px = max_offset;
                SDL_Rect crop = {px, 0, avail_w, list_scroll_surface->h};
                SDL_Rect pos = {offset_x, item_bg_rect.y + label_y - list_scroll_surface->h / 2};
                SDL_BlitSurface(list_scroll_surface, &crop, screen, &pos);

                // Advance the scroll counter; wrap back after the end-of-label pause
                list_scroll_x += LIST_LABEL_SCROLL_SPEED;
                if (list_scroll_x > max_offset + LIST_LABEL_SCROLL_PAUSE * LIST_LABEL_SCROLL_SPEED)
                    list_scroll_x = -(LIST_LABEL_SCROLL_DELAY * LIST_LABEL_SCROLL_SPEED);
            }
            else {
                // Label fits within the available space: render statically, no scroll
                theme_renderListLabel(screen, item->label, theme()->list.color,
                                      offset_x, item_bg_rect.y + label_y,
                                      true, label_end, show_disabled);
            }
        }
        else {
            theme_renderListLabel(screen, item->label, theme()->list.color,
                                  offset_x, item_bg_rect.y + label_y,
                                  list->active_pos == i, label_end, show_disabled);
        }

        if (!list_small && strlen(item->description)) {
            theme_renderListLabel(
                screen, item->description, theme()->grid.color, offset_x,
                item_bg_rect.y + 62 * g_scale, list->active_pos == i, label_end, show_disabled);
        }
    }

    // If no preview is active this frame, release any cached scroll surface so
    // memory is not retained after the user navigates away from a previewed item.
    if (active_preview == NULL && list_scroll_surface != NULL) {
        SDL_FreeSurface(list_scroll_surface);
        list_scroll_surface = NULL;
    }

    if (active_preview != NULL) {
        int preview_width = (double)params.preview_width * g_scale;
        int preview_x = 640.0 * g_scale;

        if (params.preview_bg) {
            SDL_Surface *preview_bg = resource_getSurface(PREVIEW_BG);
            SDL_Rect preview_bg_rect = {640 * g_scale - preview_bg->w, 60 * g_scale};
            SDL_BlitSurface(preview_bg, NULL, screen, &preview_bg_rect);
            preview_x -= preview_bg->w;
        }
        else {
            preview_x -= preview_width;
        }

        SDL_Surface *preview = (SDL_Surface *)active_preview->preview_ptr;
        if (preview) {
            bool free_after = false;

            if (preview->w > preview_width || (params.preview_stretch && preview->w < preview_width)) {
                double scale = (double)preview_width / (double)preview->w;
                preview = zoomSurface(preview, scale, scale, params.preview_smoothing ? SMOOTHING_ON : SMOOTHING_OFF);
                free_after = true;
            }

            SDL_Rect preview_rect = {preview_x + (preview_width - preview->w) / 2,
                                     240 * g_scale - preview->h / 2};
            SDL_BlitSurface(preview, NULL, screen, &preview_rect);

            if (free_after)
                SDL_FreeSurface(preview);
        }
    }
}

void theme_renderList(SDL_Surface *screen, List *list)
{
    ListRenderParams_s params = {
        .background = theme_background(),
        .dim = {0, 60 * g_scale, 640 * g_scale, 360 * g_scale},
        .pos = {0, 60 * g_scale},
        .show_dividers = true,
        .stretch_y = false,
        .preview_bg = true,
        .preview_stretch = false,
        .preview_width = 250,
        .preview_smoothing = false,
    };

    theme_renderListCustom(screen, list, params);
}

#endif // RENDER_MENU_H__
