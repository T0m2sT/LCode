# LCOM Project - LCode

![Grade](https://img.shields.io/badge/Grade-20%2F20-1E90FF?style=for-the-badge&labelColor=21262d)
![Course](https://img.shields.io/badge/Course-LCOM-1E90FF?style=for-the-badge&labelColor=21262d)
![Year](https://img.shields.io/badge/Year-2025%2F26-1E90FF?style=for-the-badge&labelColor=21262d)

## Framework

A framework developed from the labs that enables testing of each driver based on the parameter provided at runtime.

### Features

- **Errors:** Introduced a fail function that accepts an error type and message, improving error reporting and simplifying logging
- **RTC:** Provides access to the current date and time, from year down to seconds
- **Timer:** Enables precise time measurement and interval tracking
- **Keyboard:** Captures and interprets keyboard scancodes
- **Mouse:** Handles mouse input, including movement, clicks, and scrolling
- **Video:** Supports rendering mode configuration and on-screen drawing operations
- **Serial Port:** Supports data transfer between machines (Not fully implemented and Untested)

## Project

LCode is a code editor built for Minix, providing an interactive environment for navigating the file tree and editing files.

### Features

- **File editor:** Supports writing, text selection, and page scrolling
- **File tree:** A navigable menu for traversing the file system and opening files
- **Shortcuts:** Includes essential keyboard shortcuts and support for important special characters
- **Status Bar:** Displays the current open folder, session uptime, and current time; also allows creating new files and opening files via full path
- **Mouse Support:** Both the **file tree** and **file editor** support full mouse interaction, including scrolling, clicking, and text selection

### Shortcuts

- **Ctrl + b:** Opens and closes file tree
- **Ctrl + s:** Saves file
- **Ctrl + o:** Opens file
- **Ctrl + c:** Copies selected text
- **Ctrl + v:** Pastes selected text
- **Ctrl + x:** Cut selected text
- **Ctrl + q:** Leave LCode

## Environment

After creating the framework, we quickly realized that compiling the framework and the project separately every time a change was made was neither practical nor efficient.

The development environment should adapt to our workflow, not constrain it. To achieve this, we created [`run.sh`](run.sh), a simple shell script that automates the process of building both the framework and the project. It also provides the flexibility to run either the framework for testing purposes or the project itself, streamlining development and reducing repetitive manual steps.

To run the project simply do:
```sh
./run.sh proj
```

To test a single driver from the framework:
```sh
./run.sh fw <driver>
```


## Declaration of Responsible AI Use

We declare that:

1. We are responsible for all code and documentation in this repository, and we understand that we must be able to explain and justify any part of it on request.  
2. We have used AI-based tools (e.g., code assistants, chatbots, or generators) only to support my learning, not to bypass the intended learning outcomes or any assessment rules.  
3. Wherever AI tools contributed to this work, we have:  
   - Used them within the limits set by the course policies and institutional regulations.  
   - Reviewed, tested, and, where necessary, edited the outputs, taking full responsibility for their correctness, originality, and legality.  
   - Ensured that no confidential, personal, or sensitive data were shared with AI tools.  
4. We have not used AI tools to generate complete solutions that we present as entirely our own unaided work, and we have avoided plagiarism, whether from AI outputs or other sources.  
5. If asked, we will provide details of which tools we used, for which files or parts of the project, and how we verified and adapted their outputs.

Signed: `Luís Costa`, `Miguel Rocha`, `Pedro Teixeira`, `Rafael Silva`  
Date: `05/06/2025`

## Authors and acknowledgment

LCOM Project for group GRUPO_2LEIC14_2.

### Group members:

- Luís Costa (up202404078@up.pt)
- Miguel Rocha (up202405484@up.pt)
- Pedro Teixeira (up202404987@up.pt)
- Rafael Silva (up202406334@up.pt)

