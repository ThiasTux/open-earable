#include "Audio_Player.h"

#include <utility>

uint8_t AUDIO_BUFFER[audio_b_size * audio_b_count] __attribute__((aligned (16)));

Audio_Player::Audio_Player() {
    _tone_player = new Tone();
}

Audio_Player::~Audio_Player() {
    delete[] _tone_player;
}

bool Audio_Player::init() {
    i2s_setup();
    _tone_player->setup(); // Tone setup after i2s_setup!
    return sd_setup();
}

void Audio_Player::i2s_setup() {
    // i2s_player.setBlockBufferSizes(_blockSize, _blockCount);
    i2s_player.setBuffer(AUDIO_BUFFER, audio_b_size, audio_b_count);
}

bool Audio_Player::sd_setup() {
    if(!sd_manager.begin()) return false;
    open_file();
    return _file.isOpen();
}

void Audio_Player::update() {
    if (!_stream) return;

    if (_tone) {
        _tone_player->update();
        return;
    }

    if (check_completed()) return;
    if (!i2s_player.available()) return;


    unsigned int read = sd_to_buffer();
    if (read == 0) {
        i2s_player.completed();
    }
}

int Audio_Player::update_contiguous(int max_cont) {
    if (!_stream) return 0;

    if (_tone) {
        _tone_player->update();
        return 0;
    }

    if (check_completed()) return 0;

    int blocks = i2s_player.get_contiguous_blocks();

    int cont = min(blocks, max_cont);

    if (!cont) return 0;

    unsigned int read = sd_to_buffer(cont);

    if (read == 0) {
        i2s_player.completed();
    }
    return cont;
}

void Audio_Player::set_file(int file_num) {
    _selected = file_num;
}

void Audio_Player::start_tone(int frequency) {
    if (_stream) {
        return;
    }
    _tone = true;
    _stream = true;

    i2s_player.clear_buffer();

    _tone_player->start(frequency);
    i2s_player.start();
    play();
}

void Audio_Player::start() {
    if (_stream) {
        return;
    }
    _tone = false;
    _stream = true;

    i2s_player.clear_buffer();

    if (!open_file()) {
        return;
    }

    _cur_read_sd = _default_offset;
    _file.seekSet(_default_offset);

    i2s_player.reset_buffer();
    preload_buffer();
    i2s_player.start();
    play();
}

void Audio_Player::end() {
    if (!_stream) {
        return;
    }
    _stream = false;

    stop();
    i2s_player.end();
    if (_tone) {
        _tone_player->end();
    }
}

void Audio_Player::set_name(String name) {
    _prefix = std::move(name);
    _opened = false;
}

void Audio_Player::play() {
    i2s_player.play();
}

void Audio_Player::stop() {
    i2s_player.stop();
}

void Audio_Player::pause() {
    i2s_player.pause();
}

void Audio_Player::preload_buffer() {
    for (int i = 0; i < _preload_blocks; ++i) {
        if (_tone) {
            _tone_player->update();
        } else {
            sd_to_buffer();
        }
    }
}

unsigned int Audio_Player::sd_to_buffer() {
    uint8_t * ptr = i2s_player.getWritePointer();

    unsigned int read =sd_manager.read_block(&_file, ptr, audio_b_size);
    _cur_read_sd += read;

    if (read != 0) {
        i2s_player.incrementWritePointer();
    }
    return read;
}

unsigned int Audio_Player::sd_to_buffer(int multi) {
    uint8_t * ptr = i2s_player.getWritePointer();

    unsigned int read_total = 0;
    unsigned int read = sd_manager.read_block(&_file, ptr, audio_b_size * multi);
    _cur_read_sd += read;
    read_total += read;

    if (read != 0) {
        for (int i = 0; i < multi; ++i) {
            i2s_player.incrementWritePointer();
        }
    }

    return read_total;
}

bool Audio_Player::check_completed() {
    if (i2s_player.get_completed() && i2s_player.get_end()) {
        end();
        return true;
    }
    return false;
}

int Audio_Player::get_max_file() {
    return max_file;
}

int Audio_Player::get_max_frequency() {
    return _tone_player->get_max_frequency();
}

int Audio_Player::get_min_frequency() {
    return _tone_player->get_min_frequency();
}

bool Audio_Player::is_mode_tone() {
    return _tone;
}

unsigned int Audio_Player::get_sample_rate() {
    if (!open_file()) return 0;
    uint64_t pos = _file.curPosition();
    unsigned int rate;
    sd_manager.read_block_at(&_file, 24, (uint8_t *) &rate, 4);
    _file.seekSet(pos);
    return rate;
}

unsigned int Audio_Player::get_size() {
    if (!open_file()) return 0;
    uint64_t pos = _file.curPosition();
    return _file.fileSize();
}

int Audio_Player::ready_blocks() {
    return i2s_player.available();
}

bool Audio_Player::open_file() {
    if (_opened) return true;
    String name = _prefix + "_" + String(_selected) + _suffix;
    _file = sd_manager.openFile(name, false);
    _opened = _file.isOpen();
    return _opened;
}


void Audio_Player::config_callback(SensorConfigurationPacket *config) {
    // Check for PLAYER ID
    if (config->sensorId != PLAYER) return;

    // Get tone frequency
    int tone = int(config->sampleRate);

    audio_player.end();

    // End playback if sample tone is 0
    if (tone == 0) {
        return;
    }

    // tone <= max_file > => Play file else play tone
    if (tone <= max_file) {
        i2s_player.set_mode_file(true);
        audio_player.set_file(tone);
        audio_player.start();
    } else {
        i2s_player.set_mode_file(false);
        audio_player.start_tone(tone);
    }
}

Audio_Player audio_player;
