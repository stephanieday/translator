#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* Fehlercodes
   Eingaben formal gueltig und jedes Wort im Text konnte uebersetzt werden: 0
   Eingaben formal gueltig aber manche Woerter konnten nicht uebersetzt werden: 1
   Etwas beim oeffnen/aufrufen des WB hat nicht funktioniert: 2
   Memory Allocation failed: 3 
   WB erfuellt nicht die Spezifikationen: 4
   Fehler bei stdin: 5
*/ 

unsigned int returnCode = 0;

typedef struct 
{
   size_t wordCount;
   size_t dictLen;
   size_t sourceLen;
   size_t foreignLen;
   char **sourceLanguage;
   char **foreignLanguage;
} Dictionary;


/* Function declaration */
FILE *openWB(int argc, char* argv[]);
void storeWB(Dictionary *dictionary, FILE *fp);
void insertIntoSource(FILE *fp, char c, Dictionary *dictionary, size_t sourceIndex);
void insertIntoForeign(FILE *fp, char c, Dictionary *dictionary, size_t foreignIndex);
void createNewEntry(FILE *fp, Dictionary *dictionary);
void quicksort(FILE *fp, Dictionary *dictionary, int low, int high);
int partition(FILE *fp, Dictionary *dictionary, int low, int high);
void swap(char **word1, char **word2);
void translateText(FILE *fp, Dictionary *dictionary);
void translateWord(FILE *fp, Dictionary *dictionary, char *word);
int binarySearch(Dictionary *dictionary, int start, int end, char *searchWord);
void exitIfTranslationError(FILE *fp, Dictionary *dictionary, char *word);
void testMemoryAllocation(FILE *fp, Dictionary *dictionary, char *array);
void testMemAllocWithTwoArrays(FILE *fp, Dictionary *dictionary, char *array,  char *word);
void freeDictionary(Dictionary *dictionary);
void die(char* message, int errorCode, FILE *fp);


int main(int argc, char* argv[]) {
   /* open file */
   FILE *fp = openWB(argc, argv);
   
   Dictionary dictionary;
   storeWB(&dictionary, fp);
   
   translateText(fp, &dictionary);

   freeDictionary(&dictionary);
   fclose(fp);
   
   return returnCode;
}

/* open .wb File that was passed from command line */
FILE *openWB(int argc, char* argv[]) 
{
   if(argc != 2) 
   { 
      fprintf(stderr, "Error Message: %s", "Too few or too many command line argument.\n");
      exit(2);
   }
   
   FILE* fp = fopen(argv[1], "r");
   
   /* Test if File exists */
   if(fp == NULL) { 
      fprintf(stderr, "Error Message: %s", "File does not exist.\n");
      exit(2);
   }
   return fp;
}


/* store WB in array */
void storeWB(Dictionary *dictionary, FILE *fp) {
   dictionary->dictLen = 32;
   dictionary->sourceLen = 8;
   dictionary->foreignLen = 8;
   dictionary->wordCount = 0;
   
   /* allocate memory for array of pointers */
   dictionary->sourceLanguage = (char **)calloc(dictionary->dictLen, sizeof(char*)); 
   dictionary->foreignLanguage = (char **)calloc(dictionary->dictLen, sizeof(char*));
   
   if(dictionary->sourceLanguage == NULL || dictionary->foreignLanguage == NULL){
      free(dictionary->sourceLanguage);
      free(dictionary->foreignLanguage);
      die("Calloc failed.", 3, fp);
   }
 
  dictionary->sourceLanguage[dictionary->wordCount] = (char *) malloc(dictionary->sourceLen * sizeof(char));
  testMemoryAllocation(fp, dictionary, dictionary->sourceLanguage[dictionary->wordCount]);
  dictionary->foreignLanguage[dictionary->wordCount] = (char *) malloc(dictionary->foreignLen * sizeof(char));
  testMemoryAllocation(fp, dictionary, dictionary->foreignLanguage[dictionary->wordCount]);

   /* * * .wb in Dictionary einlesen * * */
   int status = 0;            //  status = 0 --> no word read yet, status = 1 --> a word was read so wb is not empty
   size_t sourceIndex = 0;
   size_t foreignIndex = 0;
   int countColon = 0;
   int previousChar = 0;
   char letter;

   while((letter = fgetc(fp)) != EOF) {
      if(status == 0 && letter == 10)
      {
         freeDictionary(dictionary);
         die("First Char is a Linefeed.", 4, fp);
      }
      if((97 <= letter && letter <= 122) || (letter == 10 || letter == 13 || letter == 58))
      {
         if(sourceIndex == 0 && letter == ':')
         {
            freeDictionary(dictionary);
            die("Colon is the first char in the line.", 4, fp);
         }
         if(countColon == 1 && letter == ':')
         {
            freeDictionary(dictionary);
            die("More than one colon.", 4, fp);
         }
         if(previousChar == ':' && (letter == 10 || letter == 13))
         {
            freeDictionary(dictionary);
            die("Colon at end of line.", 4, fp);
         }
         if((letter == 10 && previousChar == 10) || (letter == 13 && previousChar == 13) || (letter == 13 && previousChar == 10))
         {
            freeDictionary(dictionary);
            die("To many Line Feeds or Carriage returns.", 4, fp);
         }
         if(countColon == 0)
         {
             // Line Feed or Carriage Return but no colon before
            if((letter == 10 || letter == 13) && status == 1)
            {
               freeDictionary(dictionary);
               die("No Colon in Line.", 4, fp);
            }
            if(letter == ':')
            {
               countColon = 1;
            } 
            else 
            {
               status = 1;
               insertIntoSource(fp, letter, dictionary, sourceIndex);
               sourceIndex++;
            }
         }
         else if (countColon == 1 && letter != ':')
         {
            if(letter == 10 && previousChar == 13) 
            { 
               continue; 
            }
            if(letter == 10 || letter == 13) 
            {
               sourceIndex = 0;
               foreignIndex = 0;
               countColon = 0;
               createNewEntry(fp, dictionary);
            }
            else
            {
               insertIntoForeign(fp, letter, dictionary, foreignIndex);
               foreignIndex += 1;
            }
         } 
         else 
         {
            freeDictionary(dictionary);
            die("WB includes char(s) that are not allowed.", 4, fp);
         }
      }
      else
      {
         freeDictionary(dictionary);
         die("WB includes char(s) that are not allowed.", 4, fp);
      }
      previousChar = letter;
   }
   if((previousChar == 10 || previousChar == 13) && dictionary->wordCount > 0 && status == 1)
   {
      free(dictionary->sourceLanguage[dictionary->wordCount]);
      free(dictionary->foreignLanguage[dictionary->wordCount]);
      dictionary->wordCount--;
   }
   if(previousChar == ':')
   {
      freeDictionary(dictionary);
      die("Colon at end of Line.", 4, fp);
   }
   if(status == 1 && countColon == 0 && previousChar != 10 && previousChar != 13)
   {
      freeDictionary(dictionary);
      die("No Colon in Line.", 4, fp);
   }
   
   /* Sort Dictionary */
   quicksort(fp, dictionary, 0, dictionary->wordCount);
}


/* Insert char into source Language Array */
void insertIntoSource(FILE *fp, char c, Dictionary *dictionary, size_t sourceIndex)
{
   if(sourceIndex == dictionary->sourceLen-2)
   {
      dictionary->sourceLen *= 2;
      char *tempArray = (char *) realloc(dictionary->sourceLanguage[dictionary->wordCount], sizeof(char) * dictionary->sourceLen);  
      testMemoryAllocation(fp, dictionary, tempArray);

      dictionary->sourceLanguage[dictionary->wordCount] = tempArray;

      dictionary->sourceLanguage[dictionary->wordCount][sourceIndex] = c;
      dictionary->sourceLanguage[dictionary->wordCount][sourceIndex+1] = '\0';
   }
   else
   {
      dictionary->sourceLanguage[dictionary->wordCount][sourceIndex] = c;
      dictionary->sourceLanguage[dictionary->wordCount][sourceIndex+1] = '\0';
   }
}


/* Insert char into foreign Language Array */
void insertIntoForeign(FILE *fp, char c, Dictionary *dictionary, size_t foreignIndex)
{
   if(foreignIndex == dictionary->foreignLen-2)
   {
      dictionary->foreignLen *= 2;
      char *tempArray = (char *) realloc(dictionary->foreignLanguage[dictionary->wordCount], sizeof(char) * dictionary->foreignLen); 
      testMemoryAllocation(fp, dictionary, tempArray);
      
      dictionary->foreignLanguage[dictionary->wordCount] = tempArray;

      dictionary->foreignLanguage[dictionary->wordCount][foreignIndex] = c;
      dictionary->foreignLanguage[dictionary->wordCount][foreignIndex+1] = '\0';
   }
   else
   {
      dictionary->foreignLanguage[dictionary->wordCount][foreignIndex] = c;
      dictionary->foreignLanguage[dictionary->wordCount][foreignIndex+1] = '\0';
   }
}


/* Allocate Memory for next source and foreign language Word */
void createNewEntry(FILE *fp, Dictionary *dictionary)
{  
   dictionary->wordCount += 1;

   /* resize array of source and foreign Language */
   if(dictionary->wordCount == dictionary->dictLen-1)
   {
      dictionary->dictLen *= 2;
      char **tempSourceArray = realloc(dictionary->sourceLanguage, sizeof(char *) * dictionary->dictLen);

      if(tempSourceArray == NULL)
      {
         freeDictionary(dictionary);
         die("Realloc failed.", 3, fp);
      }
      char **tempForeignArray = realloc(dictionary->foreignLanguage, sizeof(char *) * dictionary->dictLen);
      if (tempForeignArray == NULL)
      {
         freeDictionary(dictionary);
         die("Realloc failed.", 3, fp);
      }
      else
      {
         dictionary->sourceLanguage = tempSourceArray;
         dictionary->foreignLanguage = tempForeignArray;
      }
   }

   /* allocate memory for next word & translation */
   dictionary->sourceLen = 8;
   dictionary->foreignLen = 8;

   dictionary->sourceLanguage[dictionary->wordCount] = (char *) calloc(dictionary->sourceLen, sizeof(char));
   testMemoryAllocation(fp, dictionary, dictionary->sourceLanguage[dictionary->wordCount]);
   dictionary->foreignLanguage[dictionary->wordCount] = (char *) calloc(dictionary->foreignLen, sizeof(char));
   testMemoryAllocation(fp, dictionary, dictionary->foreignLanguage[dictionary->wordCount]);
}


void quicksort(FILE *fp, Dictionary *dictionary, int low, int high)
{
   if (low < high)
   {
      /* pi is partitioning index, dictionary->sourceLanguage[high] is now 
         at right place */
      int pi = partition(fp, dictionary, low, high); 

      /* Separately sort elements before 
         partition and after partition */
      quicksort(fp, dictionary, low, pi - 1); 
      quicksort(fp, dictionary, pi + 1, high); 
   }
}


int partition(FILE *fp, Dictionary *dictionary, int low, int high)
{
   char *pivot = dictionary->sourceLanguage[high];    // pivot 
   int i = (low - 1);  // Index of smaller element 

   for(int j = low; j <= high-1; j++) 
   { 
      // If current element is smaller than or 
      // equal to pivot 
      int cmpResult = strcmp(dictionary->sourceLanguage[j], pivot);
      if(cmpResult == 0 && j != high)
      {
         freeDictionary(dictionary);
         die("Source Word is double in WB.", 4, fp);
      }
      if (cmpResult < 0) 
      { 
         i++;    // increment index of smaller element 
         
         if(i != j)
         {
            swap(&dictionary->sourceLanguage[i], &dictionary->sourceLanguage[j]); 
            swap(&dictionary->foreignLanguage[i], &dictionary->foreignLanguage[j]); 
         }
      } 
   } 
   swap(&dictionary->sourceLanguage[i + 1], &dictionary->sourceLanguage[high]); 
   swap(&dictionary->foreignLanguage[i + 1], &dictionary->foreignLanguage[high]); 
   
   // Index of Element that is at the right position now
   return (i + 1);
}


void swap(char **word1, char **word2)
{
   char *tmp = *word1;
   *word1 = *word2;
   *word2 = tmp;
}


void translateWord(FILE *fp, Dictionary *dictionary, char *word)
{
   
   if(dictionary->wordCount == 0 && strlen(*dictionary->sourceLanguage) == 0)
   {
      printf("<%s>", word);
      returnCode = 1;
   }
   else
   {
      char *stdinWordLower = (char *) malloc((strlen(word)+1) * sizeof(char));
      testMemAllocWithTwoArrays(fp, dictionary, stdinWordLower, word);

      /* lower whole word from stdin */
      for(size_t i = 0; i < strlen(word); i++)
      {
         stdinWordLower[i] = tolower(word[i]);
      }
      stdinWordLower[strlen(word)] = '\0';

      /* get index of dictionary with translation for stdin word */
      int translationIndex = binarySearch(dictionary, 0, (int) dictionary->wordCount, stdinWordLower);

      /* no translation found */
      if(translationIndex == -1)
      {
         printf("<%s>", word);
         returnCode = 1;
      }
      /* translation was found */
      else
      {
         if(isupper(word[0])) 
         {
            dictionary->foreignLanguage[translationIndex][0] -= 32;
            printf("%s", dictionary->foreignLanguage[translationIndex]);
            dictionary->foreignLanguage[translationIndex][0] += 32;
         }
         else
         {
            printf("%s", dictionary->foreignLanguage[translationIndex]);
         }
      }
      free(stdinWordLower);
   }
}


/* translate text from stdin */
void translateText(FILE *fp, Dictionary *dictionary)
{
   char letter;
   size_t sizeOfWord = 8;
   size_t indexOfChar = 0;
   size_t wordStatus = 0;

   /* Memory allocation for word read from stdin */
   char *word = (char *) malloc(sizeOfWord * sizeof(char));
   testMemoryAllocation(fp, dictionary, word);

   while((letter = getchar()) != EOF)
   {
      /* invalid stdin */
      if ((letter < 32 || letter > 126) && letter != 10)
      {
         free(word);
         freeDictionary(dictionary);
         die("Invalid characters in text from stdin.", 5, fp);
      }

      /* Non-zero if true, 0 if false */
      int isAlpha = isalpha(letter);
      if(isAlpha) 
      {
         if(indexOfChar == sizeOfWord-2)
         {
            sizeOfWord *= 2;
            char *tempWord = (char *) realloc(word, sizeOfWord * sizeof(char));
            testMemAllocWithTwoArrays(fp, dictionary, tempWord, word);

            word = tempWord;
            word[indexOfChar] = letter;
            word[indexOfChar+1] = '\0';
         }
         else
         {
            word[indexOfChar] = letter;
            word[indexOfChar+1] = '\0';
         }
         wordStatus = 1;
         indexOfChar++;
      }
      else if(isAlpha == 0 && ((letter >= 32 && letter <= 126) || letter == 10))
      {
        if(wordStatus)
        {
          translateWord(fp, dictionary, word);
          
          word[0] = '\0';
          indexOfChar = 0;
        }
        /* Non-Alphabetical letter */
        printf("%c", letter);
        wordStatus = 0;
      }
    } 
    if(wordStatus)
    {
      translateWord(fp, dictionary, word);
    }
    free(word);
}


int binarySearch(Dictionary *dictionary, int start, int end, char *searchWord)
{
   while (start <= end)
   {
      int mid = (start + end) / 2;
      
      int cmpResult = strcmp(searchWord, dictionary->sourceLanguage[mid]);

      /* Check if searchWord is equal to start/end */
      if(start == end && cmpResult != 0)
      {
         return -1;
      }
      /* If searchWord greater, search in right half */
      if (cmpResult > 0) 
      {
         start = mid + 1; 
      }
      /* If searchWord is smaller, search in left half */
      else if (cmpResult < 0) 
      {
         end = mid - 1; 
      }
      /* Check if searchWord is present at mid */
      else if (cmpResult == 0) 
      {
         return mid; 
      }
   }
   return -1;
}


void testMemoryAllocation(FILE *fp, Dictionary *dictionary, char *array)
{
  if(array == NULL)
  {
    freeDictionary(dictionary);
    die("Memory allocation failed.", 3, fp);
  }
}


void testMemAllocWithTwoArrays(FILE *fp, Dictionary *dictionary, char *array,  char *word)
{
  if(array == NULL)
  {
    free(word);
    freeDictionary(dictionary);
    die("Memory allocation failed.", 3, fp);
  }
}


void freeDictionary(Dictionary *dictionary){
  for(size_t i = 0; i <= dictionary->wordCount; i++) 
  {
    free(dictionary->foreignLanguage[i]);
    free(dictionary->sourceLanguage[i]);
    
  }
  free(dictionary->foreignLanguage);
  free(dictionary->sourceLanguage);
}

/* If an error occours this funktion prints error message an closes File */
void die(char* message, int errorCode, FILE *fp) 
{
  fprintf(stderr, "Error Message: %s\n", message);
  if(fp != NULL) fclose(fp);
  exit(errorCode);
}
