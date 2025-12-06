# 🟡 Ray Tracing in C using SDL2  
A lightweight, real-time **ray-casting simulation** built in **C** using **SDL2**.  
This project visualizes how rays propagate from a moving light source and how they interact with objects to form shadows — all computed manually with simple math.

This work is heavily inspired by **Daniel Hirsch and  fantastic explanations on the Computerphile channel**.

---

## 📸 Project Preview

![Ray Tracer Preview](https://raw.githubusercontent.com/Sho10381/Ray-Tracing_in_C/master/Preview_working(1).gif)

---

## 🚀 Features  
- Real-time ray casting  
- Rays follow the mouse for dynamic light simulation  
- Shadow formation by ray–circle intersection  
- Adjustable number of rays (via `#define RAY_NUMBER`)  
- Smooth rendering using SDL2  

---

## 📺 Learning Resources (Highly Recommended)

### ▶ Playlist That Inspired This Project  
**Ray Tracing & Rendering – Daniel Hirsch (Computerphile)**  
https://www.youtube.com/watch?v=ezXGTRSx1g8&list=PLNfbWVqqKO6bOGAyUhrJdS0gHXU9ATdUH

---

### ▶ Channels  
- **Daniel Hirsch:** https://www.youtube.com/@HirschDaniel  
- **Computerphile:** https://www.youtube.com/@Computerphile  

Please support these creators — their explanations make computer graphics much more approachable.

---

## 📁 Project Structure  

```
Ray_tracer/
│── ray_tracer.c
│── SDL2.dll
│── program.exe
└── README.md
```

---

## 🔧 Requirements  
- SDL2 (2.30.0 or newer recommended)  
- GCC / MinGW or any C compiler  
- Windows or Linux  

---

## 🛠 How to Compile (Windows — MinGW Example)

```
gcc ray_tracer.c -IC:\SDL2\include -LC:\SDL2\lib ^
-lmingw32 -lSDL2main -lSDL2 -o program.exe
```

Run it with:

```
./program.exe
```

---

## 🙏 Credits  
This project is deeply inspired by:

- **Daniel Hirsch** — for his amazing, clear explanations of ray tracing fundamentals  
- **Computerphile** — for their high-quality educational content on computer science  

Thank you to both for contributing to open knowledge and making graphics enjoyable to learn.

---

## 🧾 License  
This project is open-source.  
Feel free to study, modify, and build upon it.

---

## ⭐ Show Support  
If you liked this project, please consider giving the repo a **star** ⭐ on GitHub!
