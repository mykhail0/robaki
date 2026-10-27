"""RNG implementation for testing purposes."""


def rng(n: int) -> int:
    """RNG algorithm."""
    return (n * 279410273) % 4294967291


def main() -> None:
    """Input the seed and number of numbers to generate on separate lines and print the numbers."""
    seed = int(input())
    i = int(input())
    for _ in range(i):
        print(seed)
        seed = rng(seed)


if __name__ == "__main__":
    main()
