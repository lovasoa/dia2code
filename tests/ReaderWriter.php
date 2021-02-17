<?php

declare(strict_types=1);

require_once 'Writer.php';
require_once 'Reader.php';

interface ReaderWriter extends Writer, Reader
{
    public function read(int $characters): string;
    public function write(string $data): int;
}
