<?php

declare(strict_types=1);

interface Writer
{
    /**
     * Write as much data as possible, returning the number og characters written.
     */
    public function write(string $data): int;
}
