#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <cassert>

#include "httpd/httpd.h"

static constexpr char supported_extensions[static_cast<int>(http::contentTypes::NOT_DEFINED)][8] = {
		"html",
		"css",
		"js",
		"json"
};

static constexpr char content_header[] =
		"\n"
		"struct FilesContent {\n"
		"\tconst char *pFileName;\n"
		"\tconst char *pContent;\n"
		"\tconst uint32_t nContentLength;\n"
		"\tconst http::contentTypes contentType;\n"
		"};\n\n"
		"static constexpr struct FilesContent HttpContent[] = {\n";

static FILE *pFileContent;
static FILE *pFileIncludes;

static http::contentTypes getContentType(const char *pFileName) {
	for (int i = 0; i < static_cast<int>(http::contentTypes::NOT_DEFINED); i++) {
		const auto l = strlen(pFileName);
		const auto e = strlen(supported_extensions[i]);

		if (l > (e + 2)) {
			if (pFileName[l - e - 1] == '.') {
				if (strcmp(&pFileName[l - e], supported_extensions[i]) == 0) {
					return static_cast<http::contentTypes>(i);
				}
			}
		}
	}

	return http::contentTypes::NOT_DEFINED;
}

static int convert_to_h(const char *file_name) {
	FILE *file_in, *file_out;
	char buffer[32];
	int c, i;
	unsigned offset;

	printf("file_name=%s ", file_name);

	file_in = fopen(file_name, "r");

	if (file_in == nullptr) {
		return 0;
	}

	char *file_out_name = new char[sizeof(file_name) + 3];
	assert(file_out_name != nullptr);

	strcpy(file_out_name, file_name);
	strcat(file_out_name, ".h");

	file_out = fopen(file_out_name, "w");

	if (file_out == nullptr) {
		delete[] file_out_name;
		fclose(file_in);
		return 0;
	}

	i = snprintf(buffer, sizeof(buffer) - 1, "#include \"%s\"\n", file_out_name);
	assert(i < static_cast<int>(sizeof(buffer)));

	fwrite(buffer, sizeof(char), i, pFileIncludes);

	fwrite("static constexpr char ", sizeof(char), 22, file_out);

	char *const_name = new char[sizeof(file_name)];
	assert(const_name != NULL);

	strcpy(const_name, file_name);
	char *p = strstr(const_name, ".");
	if (p != nullptr) {
		*p = '_';
	}

	printf("const_name=%s\n", const_name);
	fwrite(const_name, sizeof(char), strlen(const_name), file_out);
	fwrite(const_name, sizeof(char), strlen(const_name), pFileContent);
	fwrite("[] = {\n", sizeof(char), 7, file_out);

	offset = 0;

	int remove_white_spaces = 1;
	int file_size = 0;

	while ((c = fgetc (file_in)) != EOF) {
		if (remove_white_spaces) {
			if (c < ' ') {
				continue;
			} else {
				remove_white_spaces = 0;
			}
		} else {
			if (c == '\n') {
				remove_white_spaces = 1;
			}
		}
		i = snprintf(buffer, sizeof(buffer) - 1, "0x%02X,%c", c, ++offset % 16 == 0 ? '\n' : ' ');
		assert(i < static_cast<int>(sizeof(buffer)));
		fwrite(buffer, sizeof(char), i, file_out);
		file_size++;
	}

	fwrite("0x00\n};\n", sizeof(char), 8, file_out);

	delete [] file_out_name;
	delete [] const_name;

	fclose(file_in);
	fclose(file_out);

	printf("file size = %d\n", file_size);

	return file_size;
}

int main() {
	pFileIncludes = fopen("includes.h", "w");
	assert(pFileIncludes != nullptr);

	pFileContent = fopen("content.h", "w");
	assert(pFileContent != nullptr);

	fwrite(content_header, sizeof(char), strlen(content_header), pFileContent);

	auto *pDir = opendir(".");

	if (pDir != nullptr) {
		struct dirent *pDirEntry;
		while ((pDirEntry = readdir(pDir)) != nullptr) {
			if (pDirEntry->d_name[0] == '.') {
				continue;
			}

			auto contentType = getContentType(pDirEntry->d_name);
			const auto isSupported = (contentType != http::contentTypes::NOT_DEFINED);
			printf("%s -> %c\n", pDirEntry->d_name, isSupported ? 'Y' : 'N');

			if (isSupported) {
				auto *pFileName = new char[strlen(pDirEntry->d_name) + 9];
				assert(pFileName != nullptr);

				auto i = snprintf(pFileName, strlen(pDirEntry->d_name) + 8, "\t{ \"%s\", ", pDirEntry->d_name);
				fwrite(pFileName, sizeof(char), i, pFileContent);
				delete[] pFileName;

				auto nContentLength = convert_to_h(pDirEntry->d_name);

				char buffer[64];
				i = snprintf(buffer, sizeof(buffer) - 1, ", %d, static_cast<http::contentTypes>(%d)", nContentLength, static_cast<int>(contentType));
				assert(i < static_cast<int>(sizeof(buffer)));
				fwrite(buffer, sizeof(char), i, pFileContent);

				fwrite(" },\n", sizeof(char), 4, pFileContent);
			}
		}

		closedir(pDir);
	}

	fclose(pFileIncludes);

	fwrite("};\n", sizeof(char), 2, pFileContent);
	fclose(pFileContent);

	system("echo \'#include <cstdint>\n\' > tmp.h");
	system("echo \'#include \"httpd/httpd.h\"\n\' >> tmp.h");
	system("cat includes.h >> tmp.h");
	system("cat content.h >> tmp.h");
	system("rm content.h");
	system("mv tmp.h content.h");

	return EXIT_SUCCESS;
}
