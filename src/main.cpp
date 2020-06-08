#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <string>
#include <vector>

//from: https://stackoverflow.com/a/27592777
#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITETO(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))
#define FIELD(data, name, index, size)             \
	inline decltype(data) name() {                 \
		return READFROM(data, index, size);        \
	}                                              \
	inline void set_##name(decltype(data) value) { \
		WRITETO(data, index, size, value);         \
	}

constexpr uint64_t random_64_bit_key = 0xF3D37B0C676EDCA6;

struct header_t {
	uint64_t key; //if equal to random_64_bit_key, image actually contains steganography (from this program)
	size_t msg_length; //determines length of originally encoded message
};

struct split_byte {
	uint8_t data;
	FIELD(data, d1, 0, 2);
	FIELD(data, d2, 2, 2);
	FIELD(data, d3, 4, 2);
	FIELD(data, d4, 6, 2);
};

struct image_bytes {
	uint8_t _1, _2, _3, _4;
	FIELD(_1, first_bits,  0, 2);
	FIELD(_2, second_bits, 0, 2);
	FIELD(_3, third_bits,  0, 2);
	FIELD(_4, fourth_bits, 0, 2);
};

uint8_t read_byte_from_image(const void* image, const unsigned offset) {
	const auto start = (image_bytes*)image + offset;
	split_byte buffer;
	buffer.set_d1(start->first_bits());
	buffer.set_d2(start->second_bits());
	buffer.set_d3(start->third_bits());
	buffer.set_d4(start->fourth_bits());
	return buffer.data;
}

auto read_entire_image(const void* image, unsigned image_size) {
	std::vector<uint8_t> bytes;
	image_size /= 4;
	for (unsigned i = 0; i < image_size; i++)
		bytes.push_back(read_byte_from_image(image, i));
	return bytes;
}

std::string get_message_from_bytes(const std::vector<uint8_t>& bytes) {
	header_t header = *(header_t*)bytes.data();
	if (header.key != random_64_bit_key)
		return "image not encoded by this program";

	std::string out(header.msg_length, ' ');
	memcpy((void*)out.data(), bytes.data() + sizeof(header_t), header.msg_length);
	return out;
}

void write_byte_to_image(const void* image, const unsigned offset, const uint8_t byte) {
	const auto start = (image_bytes*)image + offset;
	split_byte buffer = { byte };
	start->set_first_bits(buffer.d1());
	start->set_second_bits(buffer.d2());
	start->set_third_bits(buffer.d3());
	start->set_fourth_bits(buffer.d4());
}

void write_entire_message_to_image(const void* image, const unsigned image_size, const std::string& message) {
	const auto msg_len = message.length();
	const auto total_len = msg_len + sizeof(header_t);
	if (image_size / 4 < total_len) {
		std::cout << "error! message too large, use bigger picture";
		return;
	}
	const auto bytes = new uint8_t[total_len];
	*(header_t*)bytes = { random_64_bit_key, msg_len };
	memcpy(bytes + sizeof(header_t), message.data(), msg_len);
	for (unsigned i = 0; i < total_len; i++)
		write_byte_to_image(image, i, bytes[i]);
}

int main(int argc, char** argv) {
	if (argc < 3 || (strcmp(argv[1], "-e") != 0 && strcmp(argv[1], "-d") != 0)) {
		printf("incorrect usage, usage:\n");
		printf("encode image and output to same file:   main.exe -e input.png \"message\"\n");
		printf("encode image and output to diff file:   main.exe -e input.png output.png \"message\"\n");
		printf("decode image (encoded by this program): main.exe -d input.png\n");
		printf("Press enter to continue . . .");
		getchar();
		return 1;
	}

	int width, height, channels;
	const auto image = stbi_load(argv[2], &width, &height, &channels, STBI_default);
	const auto image_size = width * height * channels;
	
	if (!image) {
		printf("unable to open file");
		return 0;
	}

	if (strcmp(argv[1], "-e") == 0) {
		const auto message = argc == 5 ? std::string(argv[4]) : std::string(argv[3]);
		write_entire_message_to_image(image, image_size, message);
		stbi_write_png(argc == 5 ? argv[3] : argv[2], width, height, channels, image, width * channels);
	} else {
		const auto bytes = read_entire_image(image, image_size);
		std::cout << get_message_from_bytes(bytes);
	}

	stbi_image_free(image);
}
