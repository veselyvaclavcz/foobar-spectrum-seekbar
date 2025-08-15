# Foobar2000 Spectrum Seekbar V10.0.0

A foobar2000 component that combines spectrum visualization with seekbar functionality. Features 4 visualization styles with both mono and stereo display modes.

## 🎨 Features

- **4 Visualization Styles**: Lines, Bars, Blocks, Dots
- **2 Channel Modes**: Mixed (Mono) and Stereo (Mirrored)
- **Interactive Seekbar**: Click anywhere to seek in the track
- **Right-click Menu**: Easy switching between styles and modes
- **Real-time Visualization**: Continuous 60fps spectrum analysis
- **Visual Progress**: Progress bar and position indicator overlay

## 📸 Screenshots

### Lines Style (Stereo)
![Lines Stereo](screen/Snímek%20obrazovky%202025-08-15%20171406.png)
*Smooth line visualization with mirrored stereo channels*

### Bars Style (Stereo)
![Bars Stereo](screen/Snímek%20obrazovky%202025-08-15%20171427.png)
*Traditional bar visualization with stereo channel separation*

### Blocks Style (Mono)
![Blocks Mono](screen/Snímek%20obrazovky%202025-08-15%20171438.png)
*Stacked block visualization in mono mode*

### Dots Style (Mono)
![Dots Mono](screen/Snímek%20obrazovky%202025-08-15%20171447.png)
*Point-based visualization showing frequency dots*

## 📥 Installation

1. Download `foo_spectrum_seekbar_v10-10.0.0.fb2k-component`
2. **Important**: Uninstall any previous versions first
3. Double-click the .fb2k-component file to install
4. Restart foobar2000
5. Add "Spectrum Seekbar V10" to your layout

## 🎯 Usage

- **Left-click**: Seek to any position in the track
- **Right-click**: Open menu to change visualization settings
- **Menu Options**:
  - Visualization Style: Lines/Bars/Blocks/Dots
  - Channel Mode: Mixed (Mono)/Stereo (Mirrored)

## 🔧 Technical Details

- Built with foobar2000 SDK 2025-03-07
- Real-time FFT spectrum analysis with 32 frequency bins
- Double-buffered rendering for smooth display
- Multiple track length detection methods for compatibility

## 🛠️ Building from Source

Requirements: Visual Studio 2022 + foobar2000 SDK 2025-03-07

1. Place foobar2000 SDK in `SDK-2025-03-07/` folder
2. Run `BUILD_V10.bat` to compile
3. Run `CREATE_V10_COMPONENT.bat` to package

## 📋 Files

- `spectrum_seekbar_v10.cpp` - Complete source code
- `foo_spectrum_seekbar_v10-10.0.0.fb2k-component` - Ready-to-install component
- `BUILD_V10.bat` - Build script
- `CREATE_V10_COMPONENT.bat` - Packaging script
- `screen/` - Screenshots

## ⚙️ Visualization Modes

All combinations available (8 total):

| Style | Mono Mode | Stereo Mode |
|-------|-----------|-------------|
| Lines | ✅ | ✅ |
| Bars | ✅ | ✅ |  
| Blocks | ✅ | ✅ |
| Dots | ✅ | ✅ |

## 🔄 Version

**V10.0.0** - Complete release with 4 visualization styles and menu system

## 📄 License

Built using the official foobar2000 SDK. Component provided as-is for foobar2000.

---

**Ready to use! Download the .fb2k-component file and enjoy all visualization options.**