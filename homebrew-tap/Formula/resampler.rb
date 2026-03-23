class Resampler < Formula
  desc "High-quality audio sample rate converter"
  homepage "https://github.com/jniemann66/ReSampler"
  url "https://github.com/jniemann66/ReSampler/archive/refs/tags/v2.1.2.tar.gz"
  sha256 "3fe1b51a90b479e34aed644ccfe5fa8ef0a962f580cb3076df006b5325c01091"
  license "LGPL-2.1-or-later"

  depends_on "cmake" => :build
  depends_on "fftw"
  depends_on "libsndfile"

  def install
    system "cmake", "-S", ".", "-B", "build",
                    "-DCMAKE_BUILD_TYPE=Release",
                    *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    system "#{bin}/ReSampler", "--version"
  end
end
