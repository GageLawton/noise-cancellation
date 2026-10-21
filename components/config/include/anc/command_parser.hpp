#pragma once

#include <cstddef>
#include <cstring>

namespace anc {

// Serial config commands (design doc 5.7), so settings can change without
// reflashing:
//   get <key>    set <key> <value>    save    recal    status    help
enum class Verb { Get, Set, Save, Recal, Status, Help, Invalid };

struct Command {
    Verb verb;
    char key[48];    // empty unless get/set
    char value[32];  // empty unless set
};

namespace detail {

inline bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Copies the next space-separated word of `line` (starting at *pos) into
// out. Returns its length, 0 if there is no word, or outLen if the word
// doesn't fit (out is then left empty).
inline std::size_t nextWord(const char* line, std::size_t* pos, char* out,
                            std::size_t outLen) {
    std::size_t i = *pos;
    while (line[i] != '\0' && isSpace(line[i])) {
        ++i;
    }
    std::size_t n = 0;
    while (line[i] != '\0' && !isSpace(line[i])) {
        if (n + 1 >= outLen) {
            out[0] = '\0';
            return outLen;
        }
        out[n++] = line[i++];
    }
    out[n] = '\0';
    *pos   = i;
    return n;
}

}  // namespace detail

// Turns one typed line into a Command. Extra spaces and a trailing \r\n
// are fine. Unknown verbs, missing or extra words, and keys or values
// that are too long give Verb::Invalid. No allocation.
inline Command parseCommand(const char* line) {
    Command cmd{Verb::Invalid, {}, {}};
    Command invalid{Verb::Invalid, {}, {}};
    if (line == nullptr)
        return invalid;

    std::size_t pos = 0;
    char verb[8];
    const std::size_t verbLen = detail::nextWord(line, &pos, verb, sizeof(verb));
    if (verbLen == 0 || verbLen == sizeof(verb))
        return invalid;

    std::size_t wantWords = 0;  // words after the verb
    if (std::strcmp(verb, "get") == 0) {
        cmd.verb  = Verb::Get;
        wantWords = 1;
    } else if (std::strcmp(verb, "set") == 0) {
        cmd.verb  = Verb::Set;
        wantWords = 2;
    } else if (std::strcmp(verb, "save") == 0) {
        cmd.verb = Verb::Save;
    } else if (std::strcmp(verb, "recal") == 0) {
        cmd.verb = Verb::Recal;
    } else if (std::strcmp(verb, "status") == 0) {
        cmd.verb = Verb::Status;
    } else if (std::strcmp(verb, "help") == 0) {
        cmd.verb = Verb::Help;
    } else {
        return invalid;
    }

    if (wantWords >= 1) {
        const std::size_t n = detail::nextWord(line, &pos, cmd.key, sizeof(cmd.key));
        if (n == 0 || n == sizeof(cmd.key))
            return invalid;
    }
    if (wantWords >= 2) {
        const std::size_t n =
            detail::nextWord(line, &pos, cmd.value, sizeof(cmd.value));
        if (n == 0 || n == sizeof(cmd.value))
            return invalid;
    }

    // Nothing may follow the expected words.
    char extra[2];
    if (detail::nextWord(line, &pos, extra, sizeof(extra)) != 0)
        return invalid;

    return cmd;
}

}  // namespace anc
