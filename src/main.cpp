#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <string>
#include <vector>

constexpr uint64_t random_64_bit_key = 0xF3D37B0C676EDCA6;

struct header_t {
	uint64_t magic;      //used as format indicator (see random_64_bit_key above)
	uint64_t msg_length; //if present, dictates length of originally encoded message
};

using bool_vec_t = std::vector<bool>;

uint8_t read_byte_from_image(const bool_vec_t& image, const unsigned offset) {
	bool_vec_t out(8);
	const auto start = offset * 4 * 8;
	for (int i = 0; i < 4; i++) {
		out[i * 2] = image[start + (i * 8) + 0];
		out[i * 2 + 1] = image[start + (i * 8) + 1];
	}
	return *(uint8_t*)out._Myvec.data();
}

auto read_entire_image(const bool_vec_t& image, unsigned image_size) {
	std::vector<uint8_t> bytes;
	image_size /= 4;
	for (unsigned i = 0; i < image_size; i++)
		bytes.push_back(read_byte_from_image(image, i));
	return bytes;
}

std::string get_message_from_bytes(const std::vector<uint8_t>& bytes) {
	header_t header = *(header_t*)bytes.data();
	if (header.magic != random_64_bit_key)
		return "image not encoded by this program";
	std::string out(header.msg_length, ' ');
	memcpy((void*)out.data(), bytes.data() + sizeof(header_t), header.msg_length);
	return out;
}

void write_byte_to_image(bool_vec_t& image, const unsigned offset, const uint8_t byte) {
	bool_vec_t out(8);
	*(uint8_t*)out._Myvec.data() = byte;
	const auto start = offset * 4 * 8;
	for (int i = 0; i < 4; i++) {
		image[start + (i * 8) + 0] = out[i * 2];
		image[start + (i * 8) + 1] = out[i * 2 + 1];
	}
}

void write_entire_message_to_image(bool_vec_t& image, const unsigned image_size, const std::string& message) {
	const auto msg_len = message.length();
	const auto total_len = msg_len + sizeof(header_t);
	if (image_size / 4 < total_len)
		throw std::exception("error! message too large, use bigger picture");
	const auto bytes = new uint8_t[total_len];
	*(header_t*)bytes = { random_64_bit_key, msg_len };
	memcpy(bytes + sizeof(header_t), message.data(), msg_len);
	for (unsigned i = 0; i < total_len; i++)
		write_byte_to_image(image, i, bytes[i]);
	delete[] bytes;
}

int main(int argc, char** argv) {
	if (argc < 3 || (strcmp(argv[1], "-e") != 0 && strcmp(argv[1], "-d") != 0)) {
		printf("incorrect usage:\n");
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

	bool_vec_t image_bits(image_size * 8);
	memcpy(image_bits._Myvec.data(), image, image_size);

	if (strcmp(argv[1], "-e") == 0) {
		const auto message = argc == 5 ? std::string(argv[4]) : std::string(argv[3]);
		write_entire_message_to_image(image_bits, image_size, message);
		stbi_write_png(argc == 5 ? argv[3] : argv[2], width, height, channels, image_bits._Myvec.data(), width * channels);
	}
	else {
		const auto bytes = read_entire_image(image_bits, image_size);
		std::cout << get_message_from_bytes(bytes);
	}

	stbi_image_free(image);
}
