#!/bin/bash
# ============================================
# RGB Guardian - Sound Generator Script
# Generates basic sound effects using sox
# ============================================

echo "==================================="
echo "RGB Guardian - Sound Generator"
echo "==================================="

# Check if sox is installed
if ! command -v sox &>/dev/null; then
  echo "❌ sox is not installed!"
  echo "Installing sox..."
  sudo apt install -y sox

  if [ $? -ne 0 ]; then
    echo "Failed to install sox. Please install manually:"
    echo "  sudo apt install sox"
    exit 1
  fi
fi

echo "✅ sox is installed"

# Create assets directory if it doesn't exist
mkdir -p assets
cd assets

echo ""
echo "Generating sound effects..."
echo ""

# 1. Correct sound (pleasant high beep)
echo "🎵 Creating correct.wav (success sound)..."
sox -n correct.wav synth 0.15 sine 800 fade 0 0.15 0.05
if [ $? -eq 0 ]; then
  echo "   ✅ correct.wav created"
else
  echo "   ❌ Failed to create correct.wav"
fi

# 2. Wrong sound (harsh low buzz)
echo "🎵 Creating wrong.wav (error sound)..."
sox -n wrong.wav synth 0.3 square 150 fade 0 0.3 0.1
if [ $? -eq 0 ]; then
  echo "   ✅ wrong.wav created"
else
  echo "   ❌ Failed to create wrong.wav"
fi

# 3. Miss sound (descending sad tone)
echo "🎵 Creating miss.wav (game over sound)..."
sox -n miss.wav synth 0.4 sine 400:200 fade 0 0.4 0.15
if [ $? -eq 0 ]; then
  echo "   ✅ miss.wav created"
else
  echo "   ❌ Failed to create miss.wav"
fi

# 4. Level up sound (ascending celebratory tone)
echo "🎵 Creating levelup.wav (level up sound)..."
sox -n levelup.wav synth 0.25 sine 523.25 synth 0.25 sine 659.25 synth 0.25 sine 783.99 fade 0 0.25 0.1
if [ $? -eq 0 ]; then
  echo "   ✅ levelup.wav created"
else
  echo "   ❌ Failed to create levelup.wav"
fi

# 5. Background music (simple loop)
echo "🎵 Creating bg_music.ogg (background music)..."
# Create a simple cheerful melody
sox -n -r 44100 temp1.wav synth 0.5 sine 523.25 # C5
sox -n -r 44100 temp2.wav synth 0.5 sine 659.25 # E5
sox -n -r 44100 temp3.wav synth 0.5 sine 783.99 # G5
sox -n -r 44100 temp4.wav synth 0.5 sine 659.25 # E5

# Concatenate notes
sox temp1.wav temp2.wav temp3.wav temp4.wav temp1.wav temp2.wav temp3.wav temp4.wav temp_melody.wav

# Convert to OGG with fade
sox temp_melody.wav bg_music.ogg fade 0.1 4.0 0.1 repeat 3

# Clean up temporary files
rm -f temp*.wav

if [ $? -eq 0 ]; then
  echo "   ✅ bg_music.ogg created"
else
  echo "   ⚠️  Background music creation had issues (you can skip this)"
fi

cd ..

echo ""
echo "==================================="
echo "✅ Sound generation complete!"
echo "==================================="
echo ""
echo "Generated files:"
ls -lh assets/

echo ""
echo "You can test the sounds with:"
echo "  play assets/correct.wav"
echo "  play assets/wrong.wav"
echo "  play assets/miss.wav"
echo "  play assets/bg_music.ogg"
echo ""
echo "Now run your game:"
echo "  make run"
echo ""
