CXX      := g++
CXXFLAGS := -Wall -I.
# リポジトリルート配下の bin ディレクトリに配置する
BIN_DIR  := ../../bin

# ライブラリ相当のソース
LIB_SOURCES := \
	grovepi.cpp \
	grove_rgb_lcd/grove_rgb_lcd.cpp \
	grove_dht_pro/grove_dht_pro.cpp

LIB_OBJECTS := $(LIB_SOURCES:.cpp=.o)

# 単体のサンプル (ルート直下の .cpp)
SIMPLE_EXAMPLES := \
	grove_button \
	grove_buzzer \
	grove_led_blink \
	grove_led_fade \
	grove_light_sensor \
	grove_relay \
	grove_rotary_angle_sensor \
	grove_sound_sensor \
	grovepi_analog_read \
	grovepi_analog_write \
	grovepi_digital_read \
	grovepi_digital_write \
	grovepi_us_read

# サブディレクトリ配下のサンプル
SPECIAL_EXAMPLES := \
	grove_rgb_lcd_example \
	grove_dht_example

ALL_EXAMPLES := $(SIMPLE_EXAMPLES) $(SPECIAL_EXAMPLES)
ALL_TARGETS  := $(ALL_EXAMPLES:%=$(BIN_DIR)/%.out)

.PHONY: all clean

all: $(BIN_DIR) $(ALL_TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# ルート直下のシンプルなサンプル
$(SIMPLE_EXAMPLES:%=$(BIN_DIR)/%.out): $(BIN_DIR)/%.out: %.cpp $(LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# LCD サンプル
$(BIN_DIR)/grove_rgb_lcd_example.out: grove_rgb_lcd/grove_rgb_lcd_example.cpp $(LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# DHT サンプル
$(BIN_DIR)/grove_dht_example.out: grove_dht_pro/grove_dht_example.cpp $(LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(LIB_OBJECTS) $(ALL_TARGETS)
	rm -rf $(BIN_DIR)


