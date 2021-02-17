<?php

declare(strict_types=1);

require_once 'ReaderWriter.php';

/**
 * Represents an manipulatable file.
 */
class File implements ReaderWriter
{
    public string $filename;

    public function read(int $characters): string
    {
        throw new Error('read() not implemented');
    }
    public function write(string $data): int
    {
        throw new Error('write() not implemented');
    }
}
