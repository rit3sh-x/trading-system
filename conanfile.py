from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class TradingSystemRecipe(ConanFile):
    name = "trading-system"
    version = "0.1.0"
    package_type = "application"

    license = "MIT"
    author = "Ritesh Kumar <ritesh13327@gmail.com>"
    url = "https://github.com/rit3sh-x/trading-system"
    description = "A high-performance C++ trading system."
    topics = ("trading", "finance", "cpp")

    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "fmt/12.1.0",
        "spdlog/1.17.0",
        "tomlplusplus/3.4.0",
        "boost/1.91.0",
    )

    test_requires = (
        "catch2/3.15.3",
        "benchmark/1.9.5",
    )

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeDeps(self).generate()
        CMakeToolchain(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
