{
  description = "camera-things: Pi camera server for the OBC";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (pkgs: {
        # libcamera with only the virtual pipeline handler, patched to output
        # frames laid out like the Pi's (YUV420, padded strides) so raspy can
        # run its real camera code path without the hardware.
        libcamera-virtual = pkgs.libcamera.overrideAttrs (old: {
          pname = "libcamera-virtual";
          patches = (old.patches or [ ]) ++ [ ./nix/libcamera-virtual-pi-yuv420.patch ];
          buildInputs = old.buildInputs ++ [ pkgs.libyuv ];
          mesonFlags = old.mesonFlags ++ [ "-Dpipelines=virtual" ];
        });
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = [
            pkgs.cmake
            pkgs.pkg-config
            self.packages.${pkgs.stdenv.hostPlatform.system}.libcamera-virtual
            pkgs.libevent
            pkgs.libusb1
            pkgs.iproute2
          ];
          # Lets raspy find the virtual camera (and images/mock) from anywhere
          shellHook = ''
            export LIBCAMERA_VIRTUAL_CONFIG="$(git rev-parse --show-toplevel 2>/dev/null || pwd)/nix/virtual.yaml"
          '';
        };
      });
    };
}
