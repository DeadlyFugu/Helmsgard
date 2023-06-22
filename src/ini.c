#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_SECTIONS 256
#define MAX_PROPERTIES 1024
#define MAX_KEY_LENGTH 32
#define MAX_VALUE_LENGTH 256
#define MAX_LINE_LENGTH 256

// ===== [[ Local Types ]] =====

typedef struct {
    char name[MAX_KEY_LENGTH];
    int firstProperty;
} IniSection;

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int nextProperty;
} IniProperty;

// ===== [[ Declarations ]] =====

static bool _isWhitespace(char c);
static IniSection* _findSection(const char* name);
static IniSection* _createSection(const char* name);
static bool _setProperty(IniSection* section,
        const char* key, const char* value);
static IniProperty* _findProperty(IniSection* section, const char* key);

// ===== [[ Static Data ]] =====

static int _sectionCount;
static int _propertyCount;
static IniSection _sections[MAX_SECTIONS];
static IniProperty _properties[MAX_PROPERTIES];

// ===== [[ Implementations ]] =====

// todo: remove Ini_clear, and just clear on readFile/readAsset?
//       (this wont work with recursive calls to readFile)
//       (split body of readFile into _processFile?)
void Ini_clear(void) {
    _sectionCount = 0;
    _propertyCount = 0;
}

bool Ini_readFile(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (f == NULL) {
        Log_error("Failed to open file %s", filepath);
        return false;
    }

    IniSection* section = NULL;
    char line[MAX_LINE_LENGTH];
    while (!feof(f)) {
        // todo: check if fgets failed
        // todo: will this always be null terminated?
        line[0] = 0;
        fgets(line, MAX_LINE_LENGTH, f);

        char* begin = line;
        while (_isWhitespace(*begin)) begin++;

        if (*begin == ';') {
            // Skip comments
            continue;
        } else if (*begin == '[') {
            // Extract section name
            char* end = begin;
            while (*end != ']' && *end) end++;
            if (!*end) continue;
            *end = 0;

            // todo: check for invalid section name
            //       or duplicate section

            // Create section
            section = _createSection(&begin[1]);
            if (!section) {
                return false;
            }
        } else if (*begin == '@') {
            // Process special directives
            char command[64] = "";
            char argBuf[64] = "";
            char* parts[] = { command, argBuf };
            String_split(begin + 1, ' ', parts, 2, 64);
            const char* arg = String_trimLocal(argBuf);
            if (strcmp(command, "echo") == 0) {
                Log_info("(from %s) %s", filepath, arg);
            } else if (strcmp(command, "include") == 0) {
                char expandPath[128];
                // dirname of filepath
                char* sep = strrchr(filepath, '/');
                if (sep) {
                    char dirpath[128];
                    strncpy(dirpath, filepath, sep - filepath);
					dirpath[sep - filepath] = 0;
                    snprintf(expandPath, 128, "%s/%s", dirpath, arg);
                } else {
                    snprintf(expandPath, 128, "%s", arg);
                }
                if (!Ini_readFile(expandPath)) {
                    Log_error("(included from %s)", filepath);
                }
            } else if (strcmp(command, "extend") == 0) {
                IniSection* base = _findSection(arg);
                if (base) {
                    int propIndex = base->firstProperty;
                    while (propIndex != -1) {
                        IniProperty* property = &_properties[propIndex];
                        _setProperty(section,
                            property->key, property->value);
                        propIndex = property->nextProperty;
                    }
                } else {
                    Log_error("no such section '%s'", arg);
                }
            } else {
                Log_warn("unknown ini directive %s", command);
            }
        } else {
            // Extract property key and value
            char* end = begin;
            while (*end != '=' && *end) end++;
            if (*end != '=') continue; // todo: error if not empty
            char* begin_value = end + 1;
            while (end > begin && _isWhitespace(end[-1])) end--;
            while (_isWhitespace(*begin_value)) begin_value++;
            char* end_value = begin_value;
            char* begin_comment = strchr(begin_value, ';');
            if (begin_comment) {
                end_value = begin_comment;
            } else {
                while (*end_value) end_value++;
            }
            while (end_value > begin_value &&
                    _isWhitespace(end_value[-1])) end_value--;
            
            // Create default section if needed
            if (!section) {
                section = _createSection("");
            }

            // todo: check property isn't duplicate

            // Set property
            *end = 0;
            *end_value = 0;
            _setProperty(section, begin, begin_value);
        }
    }

    return true;
}

bool Ini_readAsset(const char* assetpath) {
    char filepath[256];
    snprintf(filepath, 256, "assets/%s", assetpath);
    return Ini_readFile(filepath);
}

bool Ini_writeFile(const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) return false;
    
    for (int i = 0; i < _sectionCount; i++) {
        IniSection* section = &_sections[i];
        if (section->name[0]) fprintf(f, "[%s]\n", section->name);
        int j = section->firstProperty;
        while (j != -1) {
            IniProperty* property = &_properties[j];
            fprintf(f, "%s=%s\n", property->key, property->value);
            j = property->nextProperty;
        }
    }

    return true;
}

const char* Ini_getSectionName(int index) {
    if (index < 0 || index >= _sectionCount) return NULL;
    return _sections[index].name;
}

const char* Ini_get(const char* section, const char* key) {
    IniSection* iniSection = _findSection(section);
    if (!iniSection) return NULL;

    IniProperty* property = _findProperty(iniSection, key);
    if (!property) return NULL;

    return property->value;
}

bool Ini_set(const char* section, const char* key, const char* value) {
    IniSection* iniSection = _findSection(section);
    if (!iniSection) {
        iniSection = _createSection(section);
        if (!iniSection) {
            return false;
        }
    }

    // todo: check value is valid

    IniProperty* property = _findProperty(iniSection, key);
    if (property) {
        strncpy(property->value, value, MAX_VALUE_LENGTH);
        return true;
    } else {
        return _setProperty(iniSection, key, value);
    }
}

static bool _isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static IniSection* _findSection(const char* name) {
    for (int i = 0; i < _sectionCount; i++) {
        if (strcmp(_sections[i].name, name) == 0) {
            return &_sections[i];
        }
    }

    return NULL;
}

static IniSection* _createSection(const char* name) {
    if (_sectionCount == MAX_SECTIONS) {
        puts("Error: too many sections");
        return NULL;
    }

    IniSection* section = &_sections[_sectionCount++];
    strncpy(section->name, name, MAX_KEY_LENGTH);
    section->firstProperty = -1;

    return section;
}

static bool _setProperty(IniSection* section,
        const char* key, const char* value) {
    if (!section) return false;
    if (_propertyCount == MAX_PROPERTIES) {
        puts("Error: too many properties");
        return false;
    }
    
    int id = _propertyCount++;
    IniProperty* property = &_properties[id];
    strncpy(property->key, key, MAX_KEY_LENGTH);
    strncpy(property->value, value, MAX_VALUE_LENGTH);
    property->nextProperty = section->firstProperty;

    section->firstProperty = id;

    return true;
}

static IniProperty* _findProperty(IniSection* section, const char* key) {
    int propIndex = section->firstProperty;
    while (propIndex != -1) {
        IniProperty* property = &_properties[propIndex];
        if (strcmp(property->key, key) == 0) {
            return property;
        }
        propIndex = property->nextProperty;
    }

    return NULL;
}
