// ProjectTwo.cpp
// ABCU Advising Assistance Program (Single-file submission)
// Command-line C++ program that loads course data, prints a sorted list, and
// shows course details with prerequisites.
//
// Build (examples):
//   g++ -std=c++17 -O2 -Wall -Wextra -pedantic ProjectTwo.cpp -o advising
//   clang++ -std=c++17 -O2 -Wall -Wextra -pedantic ProjectTwo.cpp -o advising
//
// Run:
//   ./advising
//
// The input file should be CSV with lines like:
//   CSCI100,Introduction to Computer Science
//   CSCI200,Data Structures,CSCI100
//   MATH201,Discrete Mathematics,MATH101,CSCI100
//
// Notes:
// - Titles are expected to not contain commas. (Matches the provided assignment files.)
// - Course numbers are normalized to uppercase; sorting is lexicographic/alphanumeric by course number.
// - Prereq titles are resolved if present in the file; otherwise they’re flagged as missing.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ---------- Small string helpers ----------

static inline std::string trimCopy(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

static inline void toUpperInPlace(std::string& s) {
    for (char& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
}

static std::vector<std::string> splitCSV(const std::string& line) {
    // Simple CSV split by commas (titles do not contain commas per assignment).
    std::vector<std::string> parts;
    std::string cur;
    std::istringstream iss(line);
    while (std::getline(iss, cur, ',')) {
        parts.push_back(trimCopy(cur));
    }
    return parts;
}

// ---------- Core domain model ----------

struct Course {
    std::string number;               // e.g., "CSCI200"
    std::string title;                // e.g., "Data Structures"
    std::vector<std::string> prereqs; // e.g., {"CSCI100", "MATH201"}
};

// ---------- Binary Search Tree keyed by course number ----------

struct Node {
    Course course;
    Node* left{nullptr};
    Node* right{nullptr};
    explicit Node(const Course& c) : course(c) {}
};

class CourseBST {
public:
    CourseBST() = default;
    ~CourseBST() { clear(root_); }

    void clearAll() {
        clear(root_);
        root_ = nullptr;
        count_ = 0;
    }

    size_t size() const { return count_; }

    // Insert or replace: if the key already exists, title/prereqs are updated.
    void insertOrAssign(const Course& c) {
        insertOrAssignImpl(root_, c);
    }

    const Course* find(const std::string& number) const {
        Node* cur = root_;
        while (cur) {
            if (number == cur->course.number) return &cur->course;
            if (number < cur->course.number) cur = cur->left;
            else cur = cur->right;
        }
        return nullptr;
    }

    // In-order traversal: lowest -> highest
    template <typename Fn>
    void inOrder(Fn&& fn) const {
        inOrderImpl(root_, fn);
    }

private:
    Node* root_{nullptr};
    size_t count_{0};

    static void clear(Node* n) {
        if (!n) return;
        clear(n->left);
        clear(n->right);
        delete n;
    }

    void insertOrAssignImpl(Node*& n, const Course& c) {
        if (!n) {
            n = new Node(c);
            ++count_;
            return;
        }
        if (c.number < n->course.number) {
            insertOrAssignImpl(n->left, c);
        } else if (c.number > n->course.number) {
            insertOrAssignImpl(n->right, c);
        } else {
            // Replace/assign existing node's payload (keeps tree shape stable).
            n->course.title = c.title;
            n->course.prereqs = c.prereqs;
        }
    }

    template <typename Fn>
    static void inOrderImpl(Node* n, Fn&& fn) {
        if (!n) return;
        inOrderImpl(n->left, fn);
        fn(n->course);
        inOrderImpl(n->right, fn);
    }
};

// ---------- Planner orchestrates loading, storage, and printing ----------

class CoursePlanner {
public:
    bool loadFromFile(const std::string& filename, std::string& outError) {
        std::ifstream in(filename);
        if (!in.is_open()) {
            outError = "Error: Could not open file \"" + filename + "\".";
            return false;
        }

        // Reset any previously loaded data
        tree_.clearAll();
        index_.clear();

        std::string line;
        size_t lineNum = 0;
        size_t loaded = 0;
        size_t skipped = 0;

        while (std::getline(in, line)) {
            ++lineNum;
            std::string trimmed = trimCopy(line);
            if (trimmed.empty()) continue; // allow blank lines

            auto parts = splitCSV(trimmed);
            if (parts.size() < 2) {
                std::cerr << "Warning (line " << lineNum
                          << "): expected at least course number and title. Skipping line.\n";
                ++skipped;
                continue;
            }

            // Normalize course number to uppercase for consistent keys
            std::string courseNum = parts[0];
            toUpperInPlace(courseNum);

            Course c;
            c.number = courseNum;
            c.title = parts[1];

            // Remaining parts (if any) are prerequisites
            for (size_t i = 2; i < parts.size(); ++i) {
                std::string p = parts[i];
                toUpperInPlace(p);
                if (!p.empty()) c.prereqs.push_back(p);
            }

            tree_.insertOrAssign(c);
            index_[c.number] = c.title; // keep an index for quick title lookup
            ++loaded;
        }

        if (loaded == 0) {
            outError = "Error: No valid course records were loaded from the file.";
            return false;
        }

        loaded_ = true;
        lastFilename_ = filename;

        std::cout << "Loaded " << loaded << " course(s)";
        if (skipped) std::cout << " (" << skipped << " line(s) skipped for format issues)";
        std::cout << ".\n";

        return true;
    }

    bool isLoaded() const { return loaded_; }

    void printCourseList() const {
        if (!loaded_) {
            std::cout << "Please load data first (Option 1).\n";
            return;
        }
        std::cout << "\nABCU Computer Science Course List (sorted)\n";
        std::cout << "-----------------------------------------\n";
        size_t count = 0;
        tree_.inOrder([&](const Course& c) {
            std::cout << c.number << ", " << c.title << "\n";
            ++count;
        });
        std::cout << "-----------------------------------------\n";
        std::cout << "Total: " << count << " course(s)\n\n";
    }

    void printCourseInfo(const std::string& rawNumber) const {
        if (!loaded_) {
            std::cout << "Please load data first (Option 1).\n";
            return;
        }
        std::string number = trimCopy(rawNumber);
        toUpperInPlace(number);
        if (number.empty()) {
            std::cout << "Error: course number cannot be empty.\n";
            return;
        }

        const Course* c = tree_.find(number);
        if (!c) {
            std::cout << "Course \"" << number << "\" was not found. "
                      << "Be sure you typed the correct course number (e.g., CSCI200).\n";
            return;
        }

        std::cout << "\n" << c->number << ": " << c->title << "\n";

        if (c->prereqs.empty()) {
            std::cout << "Prerequisites: None\n\n";
            return;
        }

        std::cout << "Prerequisites:\n";
        for (const std::string& p : c->prereqs) {
            auto it = index_.find(p);
            if (it != index_.end()) {
                std::cout << "  - " << p << ": " << it->second << "\n";
            } else {
                std::cout << "  - " << p << " (title not found in file)\n";
            }
        }
        std::cout << "\n";
    }

    const std::string& lastFilename() const { return lastFilename_; }

private:
    CourseBST tree_;
    std::unordered_map<std::string, std::string> index_; // courseNum -> title
    bool loaded_{false};
    std::string lastFilename_;
};

// ---------- Menu / UI loop ----------

static void printMenu() {
    std::cout << "================= ABCU Advising Assistance =================\n";
    std::cout << "1. Load data structure from file\n";
    std::cout << "2. Print an alphanumeric list of all courses\n";
    std::cout << "3. Print course information (title and prerequisites)\n";
    std::cout << "9. Exit\n";
    std::cout << "=============================================================\n";
    std::cout << "Enter your choice (1, 2, 3, or 9): ";
}

int main() {
    std::ios::sync_with_stdio(false);

    CoursePlanner planner;

    while (true) {
        printMenu();

        std::string choiceLine;
        if (!std::getline(std::cin, choiceLine)) {
            std::cout << "\nInput stream closed. Exiting.\n";
            break;
        }
        std::string choice = trimCopy(choiceLine);

        if (choice == "1") {
            std::cout << "Enter the course data filename (e.g., courses.csv): ";
            std::string fname;
            std::getline(std::cin, fname);
            fname = trimCopy(fname);

            if (fname.empty()) {
                std::cout << "Error: filename cannot be empty.\n\n";
                continue;
            }

            std::string err;
            if (!planner.loadFromFile(fname, err)) {
                std::cout << err << "\n\n";
            } else {
                std::cout << "File \"" << planner.lastFilename() << "\" loaded successfully.\n\n";
            }

        } else if (choice == "2") {
            planner.printCourseList();

        } else if (choice == "3") {
            std::cout << "Enter a course number to look up (e.g., CSCI200): ";
            std::string num;
            std::getline(std::cin, num);
            planner.printCourseInfo(num);

        } else if (choice == "9") {
            std::cout << "Goodbye!\n";
            break;

        } else {
            std::cout << "Invalid selection. Please enter 1, 2, 3, or 9.\n\n";
        }
    }

    return 0;
}
