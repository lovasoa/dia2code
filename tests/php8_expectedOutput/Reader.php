<?php

declare(strict_types=1);

interface Reader
{
    /**
     * Read up to the specified number of characters.
     */
    public function read(int $characters): string;
}
