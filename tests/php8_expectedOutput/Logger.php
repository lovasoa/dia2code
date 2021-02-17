<?php

declare(strict_types=1);

require_once 'File.php';
require_once 'Writer.php';

class Logger implements Writer
{
    /**
     * File to write log messages to.
     */
    public File $outFile;

    public $untypedAttribute;

    protected static string $applicationName = "dia2code";

    public function write(string $data): int
    {
        throw new Error('write() not implemented');
    }
}
