{
  description = "COW Interpreter Development Environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in
  {
    devShells.${system}.default = pkgs.mkShell {
      buildInputs = with pkgs; [
        cmake
        gtest
        zsh
      ];

      nativeBuildInputs = with pkgs; [
        gcc
        cmake
      ];
      
      shellHook = ''
        echo "Welcome to the COW Interpreter dev environment!"
        echo "Build tools and GTest are ready."
        export SHELL=${pkgs.zsh}/bin/zsh
        exec zsh
      '';
    };
  };
}
