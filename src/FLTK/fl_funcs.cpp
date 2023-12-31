// FLTK GUI related functions
//
// Copyright 2007-2018 by Daniel Noethen.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

#ifndef WIN32
#include <sys/wait.h>
#endif

#include "gettext.h"
#include "config.h"

#include "cfg.h"
#include "butt.h"
#include "util.h"
#include "port_audio.h"
#include "timer.h"
#include "flgui.h"
#include "fl_funcs.h"
#include "shoutcast.h"
#include "icecast.h"
#include "strfuncs.h"

int get_bitrate_index(int bitrate)
{
    int bitrate_idx[] = {8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};

    for (uint32_t i = 0; i < sizeof(bitrate_idx) / sizeof(int); i++) {
        if (bitrate == bitrate_idx[i]) {
            return i;
        }
    }

    return 0;
}

int get_bitrate_index_vorbis_vbr(int bitrate)
{
    int bitrate_idx[] = {0, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};

    for (uint32_t i = 0; i < sizeof(bitrate_idx) / sizeof(int); i++) {
        if (bitrate == bitrate_idx[i]) {
            return i;
        }
    }

    return 0;
}

void fill_cfg_widgets(void)
{
    int i;

#ifndef HAVE_LIBFDK_AAC
    fl_g->menu_item_cfg_aac->hide();
    fl_g->menu_item_rec_aac->hide();
    Fl::check();
#endif

    // fill the main section
    fl_g->choice_cfg_dev2->add(_("None"));
    for (i = 0; i < cfg.audio.dev_count; i++) {
        unsigned long dev_name_len = strlen(cfg.audio.pcm_list[i]->name) + 10;
        char *dev_name = (char *)malloc(dev_name_len);

        snprintf(dev_name, dev_name_len, "%d: %s", i, cfg.audio.pcm_list[i]->name);
        fl_g->choice_cfg_dev->add(dev_name);
        fl_g->choice_cfg_dev2->add(dev_name);
        free(dev_name);
    }

    fl_g->choice_cfg_dev->value(cfg.audio.dev_num);
    fl_g->choice_cfg_dev2->value(cfg.audio.dev2_num + 1);

    fl_g->choice_cfg_act_srv->clear();
    fl_g->choice_cfg_act_srv->redraw();
    for (i = 0; i < cfg.main.num_of_srv; i++)
        fl_g->choice_cfg_act_srv->add(cfg.srv[i]->name);

    if (cfg.main.num_of_srv > 0) {
        fl_g->button_cfg_edit_srv->activate();
        fl_g->button_cfg_del_srv->activate();
        fl_g->choice_cfg_act_srv->activate();
    }
    else {
        fl_g->button_cfg_edit_srv->deactivate();
        fl_g->button_cfg_del_srv->deactivate();
        fl_g->choice_cfg_act_srv->deactivate();
    }

    fl_g->choice_cfg_act_icy->clear();
    fl_g->choice_cfg_act_icy->redraw();
    for (i = 0; i < cfg.main.num_of_icy; i++)
        fl_g->choice_cfg_act_icy->add(cfg.icy[i]->name);

    if (cfg.main.num_of_icy > 0) {
        fl_g->button_cfg_edit_icy->activate();
        fl_g->button_cfg_del_icy->activate();
        fl_g->choice_cfg_act_icy->activate();
    }
    else {
        fl_g->button_cfg_edit_icy->deactivate();
        fl_g->button_cfg_del_icy->deactivate();
        fl_g->choice_cfg_act_icy->deactivate();
    }

    fl_g->choice_cfg_act_srv->value(cfg.selected_srv);
    fl_g->choice_cfg_act_icy->value(cfg.selected_icy);

    fl_g->check_cfg_connect->value(cfg.main.connect_at_startup);
    fl_g->check_cfg_force_reconnecting->value(cfg.main.force_reconnecting);

    fl_g->input_log_filename->value(cfg.main.log_file);

    fl_g->check_update_at_startup->value(cfg.main.check_for_update);

    fl_g->check_start_agent->value(cfg.main.start_agent);
    fl_g->check_minimize_to_tray->value(cfg.main.minimize_to_tray);

    // fill the audio section
    if (!strcmp(cfg.audio.codec, "mp3")) {
        fl_g->choice_cfg_codec->value(CHOICE_MP3);
    }
    else if (!strcmp(cfg.audio.codec, "ogg")) {
        fl_g->choice_cfg_codec->value(CHOICE_OGG);
    }
    else if (!strcmp(cfg.audio.codec, "opus")) {
        fl_g->choice_cfg_codec->value(CHOICE_OPUS);
    }
    else if (!strcmp(cfg.audio.codec, "aac")) {
        fl_g->choice_cfg_codec->value(CHOICE_AAC);
    }
    else if (!strcmp(cfg.audio.codec, "flac")) {
        fl_g->choice_cfg_codec->value(CHOICE_FLAC);
        fl_g->choice_cfg_bitrate->hide();
    }

    if (cfg.audio.dev_remember == REMEMBER_BY_ID) {
        fl_g->radio_cfg_ID->setonly();
    }
    else {
        fl_g->radio_cfg_name->setonly();
    }

    if (cfg.audio.channel == 1) {
        fl_g->choice_cfg_channel->value(CHOICE_MONO);
    }
    else {
        fl_g->choice_cfg_channel->value(CHOICE_STEREO);
    }

    fl_g->choice_cfg_bitrate->value(get_bitrate_index(cfg.audio.bitrate));

    fl_g->input_cfg_present_level->value(-cfg.audio.signal_level);
    fl_g->input_cfg_absent_level->value(-cfg.audio.silence_level);

    fl_g->input_cfg_buffer->value(cfg.audio.buffer_ms);

    fl_g->choice_cfg_resample_mode->value(cfg.audio.resample_mode);

    // Fill stream section
    fl_g->check_song_update_active->value(cfg.main.song_update);
    fl_g->check_read_last_line->value(cfg.main.read_last_line);
    fl_g->choice_cfg_song_delay->value(cfg.main.song_delay / 2);

    fl_g->input_cfg_song_file->value(cfg.main.song_path);
    fl_g->input_cfg_signal->value(cfg.main.signal_threshold);
    fl_g->input_cfg_silence->value(cfg.main.silence_threshold);

    fl_g->input_cfg_song_prefix->value(cfg.main.song_prefix);
    fl_g->input_cfg_song_suffix->value(cfg.main.song_suffix);

#if __APPLE__ && __MACH__
    fl_g->choice_cfg_app->add("iTunes\\/Music");
    fl_g->choice_cfg_app->add("Spotify");
    fl_g->choice_cfg_app->add("VOX");
    fl_g->check_cfg_use_app->value(cfg.main.app_update);
    fl_g->choice_cfg_app->value(cfg.main.app_update_service);
    if (cfg.main.app_artist_title_order == APP_ARTIST_FIRST) {
        fl_g->radio_cfg_artist_title->setonly();
    }
    else {
        fl_g->radio_cfg_title_artist->setonly();
    }
#elif (__linux__ || __FreeBSD__) && HAVE_DBUS
    fl_g->choice_cfg_app->add("Rhythmbox");
    fl_g->choice_cfg_app->add("Banshee");
    fl_g->choice_cfg_app->add("Clementine");
    fl_g->choice_cfg_app->add("Spotify");
    fl_g->choice_cfg_app->add("Cantata");
    fl_g->choice_cfg_app->add("Strawberry");
    fl_g->check_cfg_use_app->value(cfg.main.app_update);
    fl_g->choice_cfg_app->value(cfg.main.app_update_service);
    if (cfg.main.app_artist_title_order == APP_ARTIST_FIRST) {
        fl_g->radio_cfg_artist_title->setonly();
    }
    else {
        fl_g->radio_cfg_title_artist->setonly();
    }
#elif WIN32
    fl_g->choice_cfg_app->add(_("Not supported on Windows"));
    fl_g->choice_cfg_app->value(0);
    fl_g->check_cfg_use_app->value(0);
    fl_g->check_cfg_use_app->deactivate();
    fl_g->choice_cfg_app->deactivate();
    fl_g->radio_cfg_artist_title->deactivate();
    fl_g->radio_cfg_title_artist->deactivate();
#endif

    // fill the record section
    fl_g->input_rec_filename->value(cfg.rec.filename);
    fl_g->input_rec_folder->value(cfg.rec.folder);
    fl_g->input_rec_split_time->value(cfg.rec.split_time);

    if (!strcmp(cfg.rec.codec, "mp3")) {
        fl_g->choice_rec_codec->value(CHOICE_MP3);
    }
    else if (!strcmp(cfg.rec.codec, "ogg")) {
        fl_g->choice_rec_codec->value(CHOICE_OGG);
    }
    else if (!strcmp(cfg.rec.codec, "opus")) {
        fl_g->choice_rec_codec->value(CHOICE_OPUS);
    }
    else if (!strcmp(cfg.rec.codec, "aac")) {
        fl_g->choice_rec_codec->value(CHOICE_AAC);
    }
    else if (!strcmp(cfg.rec.codec, "flac")) {
        fl_g->choice_rec_codec->value(CHOICE_FLAC);
        fl_g->choice_rec_bitrate->hide();
    }
    else { // wav
        fl_g->choice_rec_codec->value(CHOICE_WAV);
        fl_g->choice_rec_bitrate->hide();
    }

    fl_g->choice_rec_bitrate->value(get_bitrate_index(cfg.rec.bitrate));

    if (cfg.rec.start_rec) {
        fl_g->check_cfg_auto_start_rec->value(1);
    }
    else {
        fl_g->check_cfg_auto_start_rec->value(0);
    }

    if (cfg.rec.stop_rec) {
        fl_g->check_cfg_auto_stop_rec->value(1);
    }
    else {
        fl_g->check_cfg_auto_stop_rec->value(0);
    }

    if (cfg.rec.rec_after_launch) {
        fl_g->check_cfg_rec_after_launch->value(1);
    }
    else {
        fl_g->check_cfg_rec_after_launch->value(0);
    }

    if (cfg.rec.sync_to_hour) {
        fl_g->check_sync_to_full_hour->value(1);
    }
    else {
        fl_g->check_sync_to_full_hour->value(0);
    }

    fl_g->input_rec_signal->value(cfg.rec.signal_threshold);
    fl_g->input_rec_silence->value(cfg.rec.silence_threshold);

    // fill the ssl/tls section
    fl_g->input_tls_cert_file->value(cfg.tls.cert_file);
    fl_g->input_tls_cert_dir->value(cfg.tls.cert_dir);

    update_samplerates_list();
    update_channel_lists();

    // fill the DSP section
    fl_g->check_activate_eq->value(cfg.dsp.equalizer);

    int choice_eq_idx;
    if (strcmp(cfg.dsp.eq_preset, "Manual") == 0) {
        choice_eq_idx = 0;
    }
    else {
        choice_eq_idx = fl_g->choice_eq_preset->find_index(cfg.dsp.eq_preset);
    }
    fl_g->choice_eq_preset->value(choice_eq_idx);
    fl_g->choice_eq_preset->do_callback();

    fl_g->check_activate_drc->value(cfg.dsp.compressor);
    fl_g->check_aggressive_mode->value(cfg.dsp.aggressive_mode);

    slider_threshold_cb(cfg.dsp.threshold);
    fl_g->thresholdSlider->value(cfg.dsp.threshold);

    slider_ratio_cb(cfg.dsp.ratio);
    fl_g->ratioSlider->value(cfg.dsp.ratio);

    slider_attack_cb(cfg.dsp.attack);
    fl_g->attackSlider->value(cfg.dsp.attack);

    slider_release_cb(cfg.dsp.release);
    fl_g->releaseSlider->value(cfg.dsp.release);

    slider_makeup_cb(cfg.dsp.makeup_gain);
    fl_g->makeupSlider->value(cfg.dsp.makeup_gain);

    // fill audio mixer
    slider_mixer_primary_device_cb(util_factor_to_db(cfg.mixer.primary_device_gain));
    fl_g->slider_mixer_primary_device->value(util_factor_to_db(cfg.mixer.primary_device_gain));

    slider_mixer_secondary_device_cb(util_factor_to_db(cfg.mixer.secondary_device_gain));
    fl_g->slider_mixer_secondary_device->value(util_factor_to_db(cfg.mixer.secondary_device_gain));

    slider_mixer_streaming_gain_cb(util_factor_to_db(cfg.mixer.streaming_gain));
    fl_g->slider_mixer_streaming_gain->value(util_factor_to_db(cfg.mixer.streaming_gain));

    slider_mixer_recording_gain_cb(util_factor_to_db(cfg.mixer.recording_gain));
    fl_g->slider_mixer_recording_gain->value(util_factor_to_db(cfg.mixer.recording_gain));

    slider_mixer_cross_fader_cb(cfg.mixer.cross_fader);
    fl_g->slider_mixer_cross_fader->value(cfg.mixer.cross_fader);

    fl_g->button_mixer_mute_primary_device->value(cfg.mixer.primary_device_muted);
    fl_g->button_mixer_mute_primary_device->do_callback();
    fl_g->button_mixer_mute_secondary_device->value(cfg.mixer.secondary_device_muted);
    fl_g->button_mixer_mute_secondary_device->do_callback();

    // fill the GUI section
    fl_g->button_gui_bg_color->color(cfg.main.bg_color, fl_lighter((Fl_Color)cfg.main.bg_color));
    fl_g->button_gui_text_color->color(cfg.main.txt_color, fl_lighter((Fl_Color)cfg.main.txt_color));
    fl_g->check_gui_attach->value(cfg.gui.attach);
    fl_g->check_gui_ontop->value(cfg.gui.ontop);
    if (cfg.gui.ontop) {
        fl_g->window_main->stay_on_top(1);
        fl_g->window_cfg->stay_on_top(1);
    }
    fl_g->check_gui_hide_log_window->value(cfg.gui.hide_log_window);
    fl_g->check_gui_remember_pos->value(cfg.gui.remember_pos);

    fl_g->check_gui_lcd_auto->value(cfg.gui.lcd_auto);

    fl_g->check_gui_start_minimized->value(cfg.gui.start_minimized);

    fl_g->check_gui_disable_gain_slider->value(cfg.gui.disable_gain_slider);
    fl_g->check_gui_disable_gain_slider->do_callback();

    fl_g->check_gui_show_listeners->value(cfg.gui.show_listeners);

    fl_g->choice_gui_language->value(lang_str_to_id(cfg.gui.lang_str));

    if (cfg.gui.vu_mode == VU_MODE_GRADIENT) {
        fl_g->radio_gui_vu_gradient->setonly();
    }
    else {
        fl_g->radio_gui_vu_solid->setonly();
    }
    
    fl_g->check_gui_always_show_vu_tabs->value(cfg.gui.always_show_vu_tabs);
    fl_g->check_gui_always_show_vu_tabs->do_callback();
    

    // fill mp3 codec settings section
    fl_g->choice_stream_mp3_enc_quality->value(cfg.mp3_codec_stream.enc_quality);
    fl_g->choice_stream_mp3_stereo_mode->value(cfg.mp3_codec_stream.stereo_mode);
    fl_g->choice_stream_mp3_bitrate_mode->value(cfg.mp3_codec_stream.bitrate_mode);
    fl_g->choice_stream_mp3_vbr_quality->value(cfg.mp3_codec_stream.vbr_quality);
    fl_g->choice_stream_mp3_vbr_min_bitrate->value(get_bitrate_index(cfg.mp3_codec_stream.vbr_min_bitrate));
    fl_g->choice_stream_mp3_vbr_max_bitrate->value(get_bitrate_index(cfg.mp3_codec_stream.vbr_max_bitrate));

    fl_g->choice_rec_mp3_enc_quality->value(cfg.mp3_codec_rec.enc_quality);
    fl_g->choice_rec_mp3_stereo_mode->value(cfg.mp3_codec_rec.stereo_mode);
    fl_g->choice_rec_mp3_bitrate_mode->value(cfg.mp3_codec_rec.bitrate_mode);
    fl_g->choice_rec_mp3_vbr_quality->value(cfg.mp3_codec_rec.vbr_quality);
    fl_g->choice_rec_mp3_vbr_min_bitrate->value(get_bitrate_index(cfg.mp3_codec_rec.vbr_min_bitrate));
    fl_g->choice_rec_mp3_vbr_max_bitrate->value(get_bitrate_index(cfg.mp3_codec_rec.vbr_max_bitrate));

    // fill vorbis codec settings section
    fl_g->choice_stream_vorbis_bitrate_mode->value(cfg.vorbis_codec_stream.bitrate_mode);
    fl_g->choice_stream_vorbis_vbr_quality->value(cfg.vorbis_codec_stream.vbr_quality);
    fl_g->choice_stream_vorbis_vbr_min_bitrate->value(get_bitrate_index_vorbis_vbr(cfg.vorbis_codec_stream.vbr_min_bitrate));
    fl_g->choice_stream_vorbis_vbr_max_bitrate->value(get_bitrate_index_vorbis_vbr(cfg.vorbis_codec_stream.vbr_max_bitrate));

    fl_g->choice_rec_vorbis_bitrate_mode->value(cfg.vorbis_codec_rec.bitrate_mode);
    fl_g->choice_rec_vorbis_vbr_quality->value(cfg.vorbis_codec_rec.vbr_quality);
    fl_g->choice_rec_vorbis_vbr_min_bitrate->value(get_bitrate_index_vorbis_vbr(cfg.vorbis_codec_rec.vbr_min_bitrate));
    fl_g->choice_rec_vorbis_vbr_max_bitrate->value(get_bitrate_index_vorbis_vbr(cfg.vorbis_codec_rec.vbr_max_bitrate));

    // fill opus codec settings section
    fl_g->choice_stream_opus_bitrate_mode->value(cfg.opus_codec_stream.bitrate_mode);
    fl_g->choice_stream_opus_quality->value(cfg.opus_codec_stream.quality);
    fl_g->choice_stream_opus_audio_type->value(cfg.opus_codec_stream.audio_type);
    fl_g->choice_stream_opus_bandwidth->value(cfg.opus_codec_stream.bandwidth);

    fl_g->choice_rec_opus_bitrate_mode->value(cfg.opus_codec_rec.bitrate_mode);
    fl_g->choice_rec_opus_quality->value(cfg.opus_codec_rec.quality);
    fl_g->choice_rec_opus_audio_type->value(cfg.opus_codec_rec.audio_type);
    fl_g->choice_rec_opus_bandwidth->value(cfg.opus_codec_rec.bandwidth);

    // fill aac codec settings section
    fl_g->choice_stream_aac_bitrate_mode->value(cfg.aac_codec_stream.bitrate_mode);
    fl_g->choice_stream_aac_afterburner->value(cfg.aac_codec_stream.afterburner);
    fl_g->choice_stream_aac_profile->value(cfg.aac_codec_stream.profile);

    fl_g->choice_rec_aac_bitrate_mode->value(cfg.aac_codec_rec.bitrate_mode);
    fl_g->choice_rec_aac_afterburner->value(cfg.aac_codec_rec.afterburner);
    fl_g->choice_rec_aac_profile->value(cfg.aac_codec_rec.profile);

    // fill flac codec settings section
    switch (cfg.flac_codec_stream.bit_depth) {
    case 16:
        fl_g->radio_stream_flac_bit_depth_16->setonly();
        break;
    case 24:
        fl_g->radio_stream_flac_bit_depth_24->setonly();
        break;
    default:
        fl_g->radio_stream_flac_bit_depth_16->setonly();
        break;
    }

    switch (cfg.flac_codec_rec.bit_depth) {
    case 16:
        fl_g->radio_rec_flac_bit_depth_16->setonly();
        break;
    case 24:
        fl_g->radio_rec_flac_bit_depth_24->setonly();
        break;
    default:
        fl_g->radio_rec_flac_bit_depth_16->setonly();
        break;
    }

#if __LITTLE_ENDIAN__ != 1 and __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#warning("24 bit wav recording has been disabled because it has not been tested on a big endian machine yet")
    fl_g->radio_rec_wav_bit_depth_24->hide();
#endif

    // fill wav codec settings section
    switch (cfg.wav_codec_rec.bit_depth) {
    case 16:
        fl_g->radio_rec_wav_bit_depth_16->setonly();
        break;
    case 24:
        fl_g->radio_rec_wav_bit_depth_24->setonly();
        break;
    case 32:
        fl_g->radio_rec_wav_bit_depth_32->setonly();
        break;
    default:
        fl_g->radio_rec_wav_bit_depth_16->setonly();
        break;
    }
}

void read_eq_slider_values(void)
{
    cfg.dsp.eq_gain[0] = fl_g->equalizerSlider1->value();
    cfg.dsp.eq_gain[1] = fl_g->equalizerSlider2->value();
    cfg.dsp.eq_gain[2] = fl_g->equalizerSlider3->value();
    cfg.dsp.eq_gain[3] = fl_g->equalizerSlider4->value();
    cfg.dsp.eq_gain[4] = fl_g->equalizerSlider5->value();
    cfg.dsp.eq_gain[5] = fl_g->equalizerSlider6->value();
    cfg.dsp.eq_gain[6] = fl_g->equalizerSlider7->value();
    cfg.dsp.eq_gain[7] = fl_g->equalizerSlider8->value();
    cfg.dsp.eq_gain[8] = fl_g->equalizerSlider9->value();
    cfg.dsp.eq_gain[9] = fl_g->equalizerSlider10->value();
}

// Updates the samplerate drop down menu for the audio
// device the user has selected
void update_samplerates_list(void)
{
    int i;
    int *sr_list;
    char sr_asc[10];

    fl_g->choice_cfg_samplerate->clear();

    sr_list = cfg.audio.pcm_list[cfg.audio.dev_num]->sr_list;

    for (i = 0; sr_list[i] != 0; i++) {
        snprintf(sr_asc, sizeof(sr_asc), "%dHz", sr_list[i]);
        fl_g->choice_cfg_samplerate->add(sr_asc);

        if (cfg.audio.samplerate == sr_list[i]) {
            fl_g->choice_cfg_samplerate->value(i);
        }
    }
    if (i == 0) {
        fl_g->choice_cfg_samplerate->add("dev. not supported");
        fl_g->choice_cfg_samplerate->value(0);
    }
}

void update_channel_lists(void)
{
    int i;
    char ch_num_txt[16];

    int dev_num = cfg.audio.dev_num;
    int num_of_channels = cfg.audio.pcm_list[dev_num]->num_of_channels;

    fl_g->choice_cfg_left_channel->clear();
    fl_g->choice_cfg_left_channel->redraw();
    fl_g->choice_cfg_right_channel->clear();
    fl_g->choice_cfg_right_channel->redraw();

    for (i = 1; i <= num_of_channels; i++) {
        snprintf(ch_num_txt, sizeof(ch_num_txt), "%d", i);
        fl_g->choice_cfg_left_channel->add(ch_num_txt);
        fl_g->choice_cfg_right_channel->add(ch_num_txt);
    }

    fl_g->choice_cfg_left_channel->value(cfg.audio.left_ch - 1);
    fl_g->choice_cfg_right_channel->value(cfg.audio.right_ch - 1);

    fl_g->choice_cfg_left_channel2->clear();
    fl_g->choice_cfg_left_channel2->redraw();
    fl_g->choice_cfg_right_channel2->clear();
    fl_g->choice_cfg_right_channel2->redraw();
    if (cfg.audio.dev2_num >= 0) {
        dev_num = cfg.audio.dev2_num;
        num_of_channels = cfg.audio.pcm_list[dev_num]->num_of_channels;

        for (i = 1; i <= num_of_channels; i++) {
            snprintf(ch_num_txt, sizeof(ch_num_txt), "%d", i);
            fl_g->choice_cfg_left_channel2->add(ch_num_txt);
            fl_g->choice_cfg_right_channel2->add(ch_num_txt);
        }
        fl_g->choice_cfg_left_channel2->value(cfg.audio.left_ch2 - 1);
        fl_g->choice_cfg_right_channel2->value(cfg.audio.right_ch2 - 1);
    }
}

void write_log(const char *message)
{
    FILE *log_fd;

    if ((cfg.main.log_file != NULL) && (strlen(cfg.main.log_file) > 0)) {
        log_fd = fl_fopen(cfg.main.log_file, "ab");
        if (log_fd != NULL) {
            fprintf(log_fd, "%s", message);
            fclose(log_fd);
        }
    }
}

void print_info(const char *info, int info_type)
{
    char timebuf[10];
    char logtimestamp[21];
    char *infotxt;
    FILE *log_fd;
    int len;

    time_t current_time;
    struct tm *current_localtime;
    static struct tm previous_localtime;

    infotxt = strdup(info);

    current_time = time(NULL);
    current_localtime = localtime(&current_time);

    if ((previous_localtime.tm_min != current_localtime->tm_min) || (previous_localtime.tm_hour != current_localtime->tm_hour)) {
        previous_localtime.tm_min = current_localtime->tm_min;
        previous_localtime.tm_hour = current_localtime->tm_hour;
        strftime(timebuf, sizeof(timebuf), "\n%H:%M:", current_localtime);
        fl_g->info_buffer->append(timebuf);
    }

    fl_g->info_buffer->append((const char *)"\n");
    fl_g->info_buffer->append((const char *)info);

    // always scroll to the last line
    fl_g->info_output->scroll(fl_g->info_buffer->count_lines(0,                            // count the lines from char 0 to the last character
                                                             fl_g->info_buffer->length()), // returns the number of characters in the buffer
                              0);

    // log to log_file if defined
    if ((cfg.main.log_file != NULL) && (strlen(cfg.main.log_file) > 0)) {
        log_fd = fl_fopen(cfg.main.log_file, "ab");
        if (log_fd != NULL) {
            strftime(logtimestamp, sizeof(logtimestamp), "%Y-%m-%d %H:%M:%S", current_localtime);
            if (strchr(infotxt, ':')) {
                strrpl(&infotxt, (char *)"\n", (char *)", ", MODE_ALL);
            }
            else {
                strrpl(&infotxt, (char *)"\n", (char *)" ", MODE_ALL);
            }

            strrpl(&infotxt, (char *)":,", (char *)": ", MODE_ALL);
            strrpl(&infotxt, (char *)"\t", (char *)"", MODE_ALL);

            len = int(strlen(infotxt)) - 1;

            if (len > 0) {
                // remove trailing commas and spaces
                while (infotxt[len] == ',' || infotxt[len] == ' ') {
                    infotxt[len--] = '\0';
                    if (len < 0) {
                        break;
                    }
                }

                fprintf(log_fd, "%s %s\n", logtimestamp, infotxt);
            }
            fclose(log_fd);
        }
    }

    free(infotxt);
}

void print_lcd(const char *text, int len, int home, int clear)
{
    if (!strcmp(text, _("idle"))) {
        fl_g->radio_co_logo->show();
    }
    else {
        fl_g->radio_co_logo->hide();
    }

    if (clear) {
        fl_g->lcd->clear();
    }

    fl_g->lcd->print((const uchar *)text, len);

    if (home) {
        fl_g->lcd->cursor_pos(0);
    }
}

int eval_record_path(int use_previous_index)
{
    FILE *fd;
    int ret;
    int has_index_var = 0;
    char *expanded_rec_folder;
    char *expanded_filename;
    char i_str[12];
    static uint32_t previous_index = 0;

    expanded_rec_folder = (char *)malloc(PATH_MAX * sizeof(char));

    // expand environment vars like ~, $HOME and %LOCALAPPDATA%
    fl_filename_expand(expanded_rec_folder, PATH_MAX, cfg.rec.folder);

    // Using %i in record folder is not allowed
    strrpl(&expanded_rec_folder, (char *)"%i", (char *)"", MODE_ALL);

    // expand fmt variables like record_folder_%d_%m_%y to record_folder_05_11_2014
    ret = expand_string(&expanded_rec_folder);
    if (ret == 0) {
        fl_alert(_("Could not create recording folder:\n%s\nPlease make sure the folder contains only valid format specifiers."), cfg.rec.folder);
        free(expanded_rec_folder);
        return 1;
    }

    // Create recording directory if it does not exist yet
    if (util_mkpath(expanded_rec_folder) != 0) {
        fl_alert(_("Could not create recording folder %s\n"), expanded_rec_folder);
        free(expanded_rec_folder);
        return 1;
    }

    expanded_filename = strdup(cfg.rec.filename);
    ret = expand_string(&expanded_filename);
    if (ret == 0) {
        fl_alert(_("Could not create recording file:\n%s\nPlease make sure the filename contains only valid format specifiers."), cfg.rec.filename);
        free(expanded_filename);
        free(expanded_rec_folder);
        return 1;
    }

    // check if there is an index variable in the filename
    if (strstr(cfg.rec.filename, "%i")) {
        has_index_var = 1;
    }

    cfg.rec.path = (char *)realloc(cfg.rec.path, (strlen(expanded_rec_folder) + strlen(expanded_filename) + 1) * sizeof(char));

    strcpy(cfg.rec.path, expanded_rec_folder);
    strcat(cfg.rec.path, expanded_filename);

    if (use_previous_index == 1) {
        snprintf(i_str, sizeof(i_str), "%d", previous_index);
        strrpl(&cfg.rec.path, (char *)"%i", i_str, MODE_ALL);
    }
    else {
        strrpl(&cfg.rec.path, (char *)"%i", (char *)"1", MODE_ALL);
        previous_index = 1;

        if ((fd = fl_fopen(cfg.rec.path, "rb")) != NULL) {
            fclose(fd);

            if (has_index_var == 1) {
                for (uint32_t i = 2; /*inf*/; i++) {
                    strcpy(cfg.rec.path, expanded_rec_folder);
                    strcat(cfg.rec.path, expanded_filename);
                    snprintf(i_str, sizeof(i_str), "%d", i);
                    strrpl(&cfg.rec.path, (char *)"%i", i_str, MODE_ALL);

                    if ((fd = fl_fopen(cfg.rec.path, "rb")) == NULL) {
                        previous_index = i;
                        break;
                    }
                    fclose(fd);

                    if (i == 0xFFFFFFFF) { // 2^32-1
                        fl_alert(_("Could not find a valid filename"));
                        free(expanded_filename);
                        free(expanded_rec_folder);
                        return 1;
                    }
                }
            }
        }
    }

    free(expanded_filename);
    free(expanded_rec_folder);

    return 0;
}

int expand_string(char **str)
{
    int str_len;
    char expanded_str[1024];
    struct tm *date;
    const time_t t = time(NULL);

    // The %i (index number) place holder must be replaced with %%i
    // Otherwise strftime will replace %i with i and the index number will loose its function
    strrpl(str, (char *)"%i", (char *)"%%i", MODE_ALL);

    // %c, %x, %X specifiers are not allowed because they return illegal characters for file names
    // Therefore we make sure that strftime will ignore them
    strrpl(str, (char *)"%c", (char *)"%%c", MODE_ALL);
    strrpl(str, (char *)"%x", (char *)"%%x", MODE_ALL);
    strrpl(str, (char *)"%X", (char *)"%%X", MODE_ALL);

    date = localtime(&t);
    strftime(expanded_str, sizeof(expanded_str) - 1, *str, date);

    str_len = strlen(expanded_str);
    *str = (char *)realloc(*str, str_len + 1);
    strncpy(*str, expanded_str, str_len + 1);
    return str_len;
}

void test_file_extension(void)
{
    char *current_ext;

    current_ext = util_get_file_extension(cfg.rec.filename);

    // Append extension
    if (current_ext == NULL) {
        cfg.rec.filename = (char *)realloc(cfg.rec.filename, strlen(cfg.rec.filename) + strlen(cfg.rec.codec) + 2);
        strcat(cfg.rec.filename, ".");
        strcat(cfg.rec.filename, cfg.rec.codec);
        fl_g->input_rec_filename->value(cfg.rec.filename);
    }
    // Replace extension
    else if (strcmp(current_ext, cfg.rec.codec)) {
        strrpl(&cfg.rec.filename, current_ext, cfg.rec.codec, MODE_LAST);
        fl_g->input_rec_filename->value(cfg.rec.filename);
    }
}

void update_codec_samplerates(void)
{
    lame_stream.samplerate = cfg.audio.samplerate;
    lame_rec.samplerate = cfg.audio.samplerate;
    lame_enc_reinit(&lame_stream);
    lame_enc_reinit(&lame_rec);

    vorbis_stream.samplerate = cfg.audio.samplerate;
    vorbis_rec.samplerate = cfg.audio.samplerate;
    vorbis_enc_reinit(&vorbis_stream);
    vorbis_enc_reinit(&vorbis_rec);

    opus_stream.samplerate = cfg.audio.samplerate;
    opus_rec.samplerate = cfg.audio.samplerate;
    opus_enc_reinit(&opus_stream);
    opus_enc_reinit(&opus_rec);

#ifdef HAVE_LIBFDK_AAC
    if (g_aac_lib_available == 1) {
        aac_stream.samplerate = cfg.audio.samplerate;
        aac_rec.samplerate = cfg.audio.samplerate;
        aac_enc_reinit(&aac_stream);
        aac_enc_reinit(&aac_rec);
    }
#endif

    flac_stream.samplerate = cfg.audio.samplerate;
    flac_rec.samplerate = cfg.audio.samplerate;
    flac_enc_reinit(&flac_stream);
    flac_enc_reinit(&flac_rec);
}

void init_main_gui_and_audio(void)
{
    int sx, sy, sw, sh;
    int butt_x, butt_y, butt_w, butt_h;
    int current_screen;

    butt_w = fl_g->window_main->w();
    butt_h = fl_g->window_main->h();

    Fl::screen_xywh(sx, sy, sw, sh);
    current_screen = Fl::screen_num(sx, sy);

    // Make sure butt is opened at screen center on first start
    if (cfg.gui.x_pos == -1 && cfg.gui.y_pos == -1) {
        Fl::screen_xywh(sx, sy, sw, sh, current_screen);
        cfg.gui.x_pos = sx + sw / 2 - butt_w / 2;
        cfg.gui.y_pos = sy + sh / 2 - butt_h / 2;
    }

    if (cfg.gui.remember_pos) {
        butt_x = cfg.gui.x_pos;
        butt_y = cfg.gui.y_pos;

        int is_visible = 0;
        for (int i = 0; i < Fl::screen_count(); i++) {
            Fl::screen_xywh(sx, sy, sw, sh, i);
            if ((butt_x >= sx - butt_w + 50) && (butt_x + 50 < (sx + sw)) && (butt_y >= sy - butt_h + 50) && (butt_y + 50 < (sy + sh))) {
                is_visible = 1;
            }
        }

        // Move butt window to the saved position only if at least 50 pixel of butt are visible on the screen
        if (is_visible) {
            fl_g->window_main->position(cfg.gui.x_pos, cfg.gui.y_pos);
        }

        if (cfg.gui.window_height > 213) {
            fl_g->window_main->resize(fl_g->window_main->x(), fl_g->window_main->y(), fl_g->window_main->w(), cfg.gui.window_height);
        }

        // if( (butt_x+butt_w > 50) && (butt_x+50 < total_screen_width) && (butt_y+butt_h > 50) && (butt_y+50 < total_screen_height) )
    }
    else { // Open at screen center if cfg.gui.remember_pos == 0
        Fl::screen_xywh(sx, sy, sw, sh, current_screen);
        fl_g->window_main->position(sx + sw / 2 - butt_w / 2, sy + sh / 2 - butt_h / 2);
    }

    fl_g->slider_gain->value(util_factor_to_db(cfg.main.gain));
    fl_g->slider_gain->do_callback();

    if (cfg.gui.ontop) {
        fl_g->window_main->stay_on_top(1);
    }

    fl_g->button_info->label(_("Hide log"));
    if (cfg.gui.hide_log_window) {
        button_info_cb();
    }

    lame_stream.channel = cfg.audio.channel;
    lame_stream.bitrate = cfg.audio.bitrate;
    lame_stream.samplerate = cfg.audio.samplerate;
    lame_stream.enc_quality = cfg.mp3_codec_stream.enc_quality;
    lame_stream.stereo_mode = cfg.mp3_codec_stream.stereo_mode;
    lame_stream.bitrate_mode = cfg.mp3_codec_stream.bitrate_mode;
    lame_stream.vbr_quality = cfg.mp3_codec_stream.vbr_quality;
    lame_stream.vbr_min_bitrate = cfg.mp3_codec_stream.vbr_min_bitrate;
    lame_stream.vbr_max_bitrate = cfg.mp3_codec_stream.vbr_max_bitrate;
    lame_enc_reinit(&lame_stream);

    lame_rec.channel = cfg.audio.channel;
    lame_rec.bitrate = cfg.rec.bitrate;
    lame_rec.samplerate = cfg.audio.samplerate;
    lame_rec.enc_quality = cfg.mp3_codec_rec.enc_quality;
    lame_rec.stereo_mode = cfg.mp3_codec_rec.stereo_mode;
    lame_rec.bitrate_mode = cfg.mp3_codec_rec.bitrate_mode;
    lame_rec.vbr_quality = cfg.mp3_codec_rec.vbr_quality;
    lame_rec.vbr_min_bitrate = cfg.mp3_codec_rec.vbr_min_bitrate;
    lame_rec.vbr_max_bitrate = cfg.mp3_codec_rec.vbr_max_bitrate;
    lame_enc_reinit(&lame_rec);

    vorbis_stream.channel = cfg.audio.channel;
    vorbis_stream.bitrate = cfg.audio.bitrate;
    vorbis_stream.samplerate = cfg.audio.samplerate;
    vorbis_stream.bitrate_mode = cfg.vorbis_codec_stream.bitrate_mode;
    vorbis_stream.vbr_quality = 1 - (cfg.vorbis_codec_stream.vbr_quality * 0.1);
    vorbis_stream.vbr_min_bitrate = cfg.vorbis_codec_stream.vbr_min_bitrate;
    vorbis_stream.vbr_max_bitrate = cfg.vorbis_codec_stream.vbr_max_bitrate;
    vorbis_enc_reinit(&vorbis_stream);

    vorbis_rec.channel = cfg.audio.channel;
    vorbis_rec.bitrate = cfg.rec.bitrate;
    vorbis_rec.samplerate = cfg.audio.samplerate;
    vorbis_rec.bitrate_mode = cfg.vorbis_codec_rec.bitrate_mode;
    vorbis_rec.vbr_quality = 1 - (cfg.vorbis_codec_rec.vbr_quality * 0.1);
    vorbis_rec.vbr_min_bitrate = cfg.vorbis_codec_rec.vbr_min_bitrate;
    vorbis_rec.vbr_max_bitrate = cfg.vorbis_codec_rec.vbr_max_bitrate;
    vorbis_enc_reinit(&vorbis_rec);

    opus_stream.channel = cfg.audio.channel;
    opus_stream.bitrate = cfg.audio.bitrate * 1000;
    opus_stream.samplerate = cfg.audio.samplerate;
    opus_stream.bitrate_mode = cfg.opus_codec_stream.bitrate_mode;
    opus_stream.audio_type = cfg.opus_codec_stream.audio_type;
    opus_stream.quality = cfg.opus_codec_stream.quality;
    opus_stream.bandwidth = cfg.opus_codec_stream.bandwidth;
    opus_enc_alloc(&opus_stream);
    opus_enc_init(&opus_stream);

    opus_rec.channel = cfg.audio.channel;
    opus_rec.bitrate = cfg.rec.bitrate * 1000;
    opus_rec.samplerate = cfg.audio.samplerate;
    opus_rec.bitrate_mode = cfg.opus_codec_rec.bitrate_mode;
    opus_rec.audio_type = cfg.opus_codec_rec.audio_type;
    opus_rec.quality = cfg.opus_codec_rec.quality;
    opus_rec.bandwidth = cfg.opus_codec_rec.bandwidth;
    opus_enc_alloc(&opus_rec);
    opus_enc_init(&opus_rec);

#ifdef HAVE_LIBFDK_AAC
    if (g_aac_lib_available == 1) {
        aac_stream.channel = cfg.audio.channel;
        aac_stream.bitrate = cfg.audio.bitrate;
        aac_stream.samplerate = cfg.audio.samplerate;
        aac_stream.bitrate_mode = cfg.aac_codec_stream.bitrate_mode;
        aac_stream.profile = cfg.aac_codec_stream.profile;
        aac_stream.afterburner = cfg.aac_codec_stream.afterburner == 0 ? 1 : 0;
        aac_enc_reinit(&aac_stream);

        aac_rec.channel = cfg.audio.channel;
        aac_rec.bitrate = cfg.rec.bitrate;
        aac_rec.samplerate = cfg.audio.samplerate;
        aac_rec.bitrate_mode = cfg.aac_codec_rec.bitrate_mode;
        aac_rec.profile = cfg.aac_codec_rec.profile;
        aac_rec.afterburner = cfg.aac_codec_rec.afterburner == 0 ? 1 : 0;
        aac_enc_reinit(&aac_rec);
    }
#endif

    flac_stream.channel = cfg.audio.channel;
    flac_stream.samplerate = cfg.audio.samplerate;
    flac_stream.enc_type = FLAC_ENC_TYPE_STREAM;
    flac_stream.bit_depth = cfg.flac_codec_stream.bit_depth;
    flac_enc_reinit(&flac_stream);

    flac_rec.channel = cfg.audio.channel;
    flac_rec.samplerate = cfg.audio.samplerate;
    flac_rec.enc_type = FLAC_ENC_TYPE_REC;
    flac_rec.bit_depth = cfg.flac_codec_rec.bit_depth;
    flac_enc_reinit(&flac_rec);
}

char *ask_user_msg = NULL;
char *ask_user_hash = NULL;
int ask_user_has_clicked = 0;
int ask_user_answer = 0;

void ask_user_reset(void)
{
    if (ask_user_msg != NULL) {
        free(ask_user_msg);
        ask_user_msg = NULL;
    }
    if (ask_user_hash != NULL) {
        free(ask_user_hash);
        ask_user_hash = NULL;
    }
    ask_user_has_clicked = 0;
}

int ask_user_get_has_clicked(void)
{
    return ask_user_has_clicked;
}

int ask_user_get_answer(void)
{
    return ask_user_answer;
}

void ask_user_set_msg(char *msg)
{
    int len = strlen(msg);
    ask_user_msg = (char *)calloc(len + 1, sizeof(char));
    strncpy(ask_user_msg, msg, len);
}

void ask_user_set_hash(char *hash)
{
    int len = strlen(hash);
    ask_user_hash = (char *)calloc(len + 1, sizeof(char));
    strncpy(ask_user_hash, hash, len);
}

void ask_user_ask(void)
{
    if (fl_choice("%s", _("TRUST"), _("No"), NULL, ask_user_msg) == 1) { // No
        ask_user_answer = IC_ABORT;
    }
    else { // TRUST
        int len = strlen(ask_user_hash);
        cfg.srv[cfg.selected_srv]->cert_hash = (char *)realloc(cfg.srv[cfg.selected_srv]->cert_hash, len + 1);

        memset(cfg.srv[cfg.selected_srv]->cert_hash, 0, len + 1);
        strncpy(cfg.srv[cfg.selected_srv]->cert_hash, ask_user_hash, len);
        ask_user_answer = IC_RETRY;
    }

    ask_user_has_clicked = 1;
}

void activate_stream_ui_elements(void)
{
    if (!recording) {
        fl_g->choice_cfg_channel->activate();
        fl_g->choice_cfg_dev->activate();
        fl_g->choice_cfg_dev2->activate();
        fl_g->choice_cfg_samplerate->activate();
        fl_g->button_cfg_rescan_devices->activate();
    }

    fl_g->choice_cfg_codec->activate();
    fl_g->choice_cfg_bitrate->activate();

    fl_g->choice_stream_mp3_enc_quality->activate();
    fl_g->choice_stream_mp3_stereo_mode->activate();
    fl_g->choice_stream_mp3_vbr_quality->activate();
    fl_g->choice_stream_mp3_bitrate_mode->activate();
    fl_g->choice_stream_mp3_vbr_min_bitrate->activate();
    fl_g->choice_stream_mp3_vbr_max_bitrate->activate();

    fl_g->choice_stream_vorbis_vbr_quality->activate();
    fl_g->choice_stream_vorbis_bitrate_mode->activate();
    fl_g->choice_stream_vorbis_vbr_min_bitrate->activate();
    fl_g->choice_stream_vorbis_vbr_max_bitrate->activate();

    fl_g->choice_stream_opus_quality->activate();
    fl_g->choice_stream_opus_audio_type->activate();
    fl_g->choice_stream_opus_bandwidth->activate();
    fl_g->choice_stream_opus_bitrate_mode->activate();

    fl_g->choice_stream_aac_profile->activate();
    fl_g->choice_stream_aac_afterburner->activate();
    fl_g->choice_stream_aac_bitrate_mode->activate();

    fl_g->radio_stream_flac_bit_depth_16->activate();
    fl_g->radio_stream_flac_bit_depth_24->activate();
}
void deactivate_stream_ui_elements(void)
{
    fl_g->choice_cfg_bitrate->deactivate();
    fl_g->choice_cfg_samplerate->deactivate();
    fl_g->choice_cfg_channel->deactivate();
    fl_g->choice_cfg_samplerate->deactivate();

    fl_g->choice_cfg_dev->deactivate();
    fl_g->choice_cfg_dev2->deactivate();
    fl_g->button_cfg_rescan_devices->deactivate();
    fl_g->choice_cfg_codec->deactivate();

    fl_g->choice_stream_mp3_enc_quality->deactivate();
    fl_g->choice_stream_mp3_stereo_mode->deactivate();
    fl_g->choice_stream_mp3_vbr_quality->deactivate();
    fl_g->choice_stream_mp3_bitrate_mode->deactivate();
    fl_g->choice_stream_mp3_vbr_min_bitrate->deactivate();
    fl_g->choice_stream_mp3_vbr_max_bitrate->deactivate();

    fl_g->choice_stream_vorbis_vbr_quality->deactivate();
    fl_g->choice_stream_vorbis_bitrate_mode->deactivate();
    fl_g->choice_stream_vorbis_vbr_min_bitrate->deactivate();
    fl_g->choice_stream_vorbis_vbr_max_bitrate->deactivate();

    fl_g->choice_stream_opus_quality->deactivate();
    fl_g->choice_stream_opus_audio_type->deactivate();
    fl_g->choice_stream_opus_bandwidth->deactivate();
    fl_g->choice_stream_opus_bitrate_mode->deactivate();

    fl_g->choice_stream_aac_profile->deactivate();
    fl_g->choice_stream_aac_afterburner->deactivate();
    fl_g->choice_stream_aac_bitrate_mode->deactivate();

    fl_g->radio_stream_flac_bit_depth_16->deactivate();
    fl_g->radio_stream_flac_bit_depth_24->deactivate();
}

void activate_rec_ui_elements(void)
{
    if (!connected) {
        fl_g->choice_cfg_channel->activate();
        fl_g->choice_cfg_dev->activate();
        fl_g->choice_cfg_dev2->activate();
        fl_g->choice_cfg_samplerate->activate();
        fl_g->button_cfg_rescan_devices->activate();
    }

    fl_g->choice_rec_codec->activate();
    fl_g->choice_rec_bitrate->activate();

    fl_g->choice_rec_mp3_enc_quality->activate();
    fl_g->choice_rec_mp3_stereo_mode->activate();
    fl_g->choice_rec_mp3_vbr_quality->activate();
    fl_g->choice_rec_mp3_bitrate_mode->activate();
    fl_g->choice_rec_mp3_vbr_min_bitrate->activate();
    fl_g->choice_rec_mp3_vbr_max_bitrate->activate();

    fl_g->choice_rec_vorbis_vbr_quality->activate();
    fl_g->choice_rec_vorbis_bitrate_mode->activate();
    fl_g->choice_rec_vorbis_vbr_min_bitrate->activate();
    fl_g->choice_rec_vorbis_vbr_max_bitrate->activate();

    fl_g->choice_rec_opus_quality->activate();
    fl_g->choice_rec_opus_audio_type->activate();
    fl_g->choice_rec_opus_bandwidth->activate();
    fl_g->choice_rec_opus_bitrate_mode->activate();

    fl_g->choice_rec_aac_profile->activate();
    fl_g->choice_rec_aac_afterburner->activate();
    fl_g->choice_rec_aac_bitrate_mode->activate();

    fl_g->radio_rec_flac_bit_depth_16->activate();
    fl_g->radio_rec_flac_bit_depth_24->activate();

    fl_g->radio_rec_wav_bit_depth_16->activate();
    fl_g->radio_rec_wav_bit_depth_24->activate();
    fl_g->radio_rec_wav_bit_depth_32->activate();
}
void deactivate_rec_ui_elements(void)
{
    fl_g->choice_rec_codec->deactivate();
    fl_g->choice_rec_bitrate->deactivate();
    fl_g->choice_cfg_channel->deactivate();
    fl_g->choice_cfg_dev->deactivate();
    fl_g->choice_cfg_dev2->deactivate();
    fl_g->choice_cfg_samplerate->deactivate();
    fl_g->button_cfg_rescan_devices->deactivate();

    fl_g->choice_rec_mp3_enc_quality->deactivate();
    fl_g->choice_rec_mp3_stereo_mode->deactivate();
    fl_g->choice_rec_mp3_vbr_quality->deactivate();
    fl_g->choice_rec_mp3_bitrate_mode->deactivate();
    fl_g->choice_rec_mp3_vbr_min_bitrate->deactivate();
    fl_g->choice_rec_mp3_vbr_max_bitrate->deactivate();

    fl_g->choice_rec_vorbis_vbr_quality->deactivate();
    fl_g->choice_rec_vorbis_bitrate_mode->deactivate();
    fl_g->choice_rec_vorbis_vbr_min_bitrate->deactivate();
    fl_g->choice_rec_vorbis_vbr_max_bitrate->deactivate();

    fl_g->choice_rec_opus_quality->deactivate();
    fl_g->choice_rec_opus_audio_type->deactivate();
    fl_g->choice_rec_opus_bandwidth->deactivate();
    fl_g->choice_rec_opus_bitrate_mode->deactivate();

    fl_g->choice_rec_aac_profile->deactivate();
    fl_g->choice_rec_aac_afterburner->deactivate();
    fl_g->choice_rec_aac_bitrate_mode->deactivate();

    fl_g->radio_rec_flac_bit_depth_16->deactivate();
    fl_g->radio_rec_flac_bit_depth_24->deactivate();

    fl_g->radio_rec_wav_bit_depth_16->deactivate();
    fl_g->radio_rec_wav_bit_depth_24->deactivate();
    fl_g->radio_rec_wav_bit_depth_32->deactivate();
}

void lang_id_to_str(int lang_id, char **lang_str, int mapping)
{
    if (lang_id >= LANG_COUNT) {
        lang_id = 0;
    }

    if (mapping == LANG_MAPPING_NEW) {
        *lang_str = (char *)realloc(*lang_str, strlen(lang_array_new[lang_id]) + 1);
        snprintf(*lang_str, strlen(lang_array_new[lang_id]) + 1, "%s", lang_array_new[lang_id]);
    }
    else { // for compatibility with butt configurations <= 0.1.33
        *lang_str = (char *)realloc(*lang_str, strlen(lang_array_old[lang_id]) + 1);
        snprintf(*lang_str, strlen(lang_array_old[lang_id]) + 1, "%s", lang_array_old[lang_id]);
    }
}

int lang_str_to_id(char *lang_str)
{
    int lang_id = 0;

    for (int i = 0; i < LANG_COUNT; i++) {
        if (strcmp(lang_str, lang_array_new[i]) == 0) {
            lang_id = i;
        }
    }

    return lang_id;
}
