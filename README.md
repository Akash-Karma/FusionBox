https://github.com/user-attachments/assets/8a5eacd8-956a-4390-a169-766b5e9ebb06

# FusionBox: Stellar Evolution 🌌

A physics-based "Suika-style" evolution game built with **C++**, **SFML 3.0**, and the **Box2D 3.0 C API**. Lead a cosmic journey from a tiny Meteor to a massive Red Giant by merging celestial bodies in a gravity-defying container.



## 🚀 Technical Highlights

* **Box2D 3.0 Integration:** Leverages the latest C-based version of Box2D for rigid-body dynamics, utilizing custom density, friction, and restitution values for realistic orbital "bounce" and rolling.
* **Stellar Progression Logic:** Features an 8-stage evolution chain with dynamic scaling:
    * *Meteor → Moon → Mercury → Mars → Earth → Jupiter → Blue Star → Red Giant*
* **Resolution Independence:** Implements `sf::View` and coordinate mapping (`mapPixelToCoords`) to ensure consistent gameplay and physics across different window sizes.
* **Advanced Graphics:** * Real-time sprite-to-physics synchronization.
    * Parallax scrolling background stars.
    * Additive blending (`sf::BlendAdd`) for star-tier objects to simulate high-energy glowing effects.
    * Dynamic screen shake upon high-level fusion events.

## 🛠️ Built With

* **Language:** C++20
* **Physics:** [Box2D 3.0](https://github.com/erincatto/box2d)
* **Graphics/Windowing:** [SFML 3.0](https://www.sfml-dev.org/)
* **Build System:** CMake

## 🎮 How to Play

1.  **Aim:** Move your mouse left and right to position the next celestial body.
2.  **Drop:** Click to release the object into the container.
3.  **Merge:** Touch two objects of the same level to evolve them into the next stage of the stellar lifecycle.
4.  **Survival:** Do not let the planets stack above the **Red Danger Line**. If they settle at the top, the system collapses!

## 🔧 Installation & Build

### Prerequisites
* C++ Compiler (GCC/MinGW, Clang, or MSVC)
* CMake 3.15+
* SFML 3.0 and Box2D 3.0 installed via your package manager (e.g., vcpkg or MSYS2)

### Building from Source
```bash
git clone [https://github.com/Akash-Karma/FusionBox.git](https://github.com/Akash-Karma/FusionBox.git)
cd FusionBox
mkdir build && cd build
cmake ..
cmake --build .
