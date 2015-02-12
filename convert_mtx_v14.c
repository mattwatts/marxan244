// type definition for relational matrix
typedef struct matrixstruct
{
		double amount, prob;
		int puno;
        int spno;
} typematrixstruct;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char sVersionString[100] = "Marxan matrix converter v1.4";
char sMattWattsEmail[100] = "m.watts@uq.edu.au";

// global debug variables
int iDebugSort = 0;
FILE *debugfp;
char sDebugLine[200];

void TrimTrailingSpaces(char sInputString[],char sOutputString[])
{
	 // if the string is like 1234.0000000 then return 1234
	 // if the string is like 1234.5670000 then return 1234.567

	 // iTrailingZeros = 0 if no trailing zeros
	 int iTrailingZeros = 0, i, iLength, iNewLength;
	 char cCharNextToZeros;

	 iLength = strlen(sInputString);

	 if (iLength > 2)
	 {
		i = iLength - 2;  // index is zero base and last string character is null

	 	while ((sInputString[i] == 48) && (i > 0))
	 	{
			  iTrailingZeros++;
			  i--;
		}
		cCharNextToZeros = sInputString[i];

		if (iTrailingZeros > 0)
		{
           // map characters to new string
           iNewLength = iLength - iTrailingZeros - 1;
           if (cCharNextToZeros == 46)
              iNewLength--;
           for (i=0;i<iNewLength;i++)
               sOutputString[i] = sInputString[i];
           // add string terminator
           sOutputString[i+1] = sInputString[iLength];
		}
		else
			strcpy(sOutputString,sInputString);
     }

}

void InsertZeroesIntoString(char sInputString[],char sOutputString[])
{
     // if 2 commas occur in string, insert a zero between them
     int i,j, iLength, iLastCharComma = 0, iThisCharComma;
     char cZero = '0';

     iLength = strlen(sInputString);

     for (i=0,j=0;i<iLength;i++,j++)
     {
         iThisCharComma = (sInputString[i] == ',');

         if (iThisCharComma && iLastCharComma)
         {
            sOutputString[j] = cZero;
            j++;
            sOutputString[j] = sInputString[i];
         }
         else
             sOutputString[j] = sInputString[i];

         iLastCharComma = iThisCharComma;
     }
     sOutputString[j] = NULL;
}

void ExecuteMatrixConversion_Tabular_To_Relational_AddSpeciesIndex(char sInputFileName[],char sOutputFileName[],
                                                                   int iBaseSpeciesIndex,int iSuppressHeaderRow,
                                                                   int iConvertM2ToHectares)
{
     int i, iSpeciesCount, iSpeciesId, iScanfResult, iPUCount = 0, iRecordCount = 0;
     FILE *infp,*outfp, *outspnamefp, *debugfp;
     char *readname,*writename;
     char sLine[20000],sAdjustedLine[2000], sElement[500], sInputString[500],sOutputString[500];
     int iPUID;
     char *sVarVal, *sVarName;
     double rAmount, rReportAmount, rRoundAmount;
     long iRoundAmount;

     printf("%s\n\n",sVersionString);
     printf("Conversion type is tabular format to relational format, add number to base species index.\n");
     printf("[input file name] %s\n",sInputFileName);
     printf("[output file name] %s\n",sOutputFileName);
     printf("[base species index] %i\n",iBaseSpeciesIndex);
     if (iSuppressHeaderRow == 1)
        printf("[suppress header row] yes\n");
     else
         printf("[suppress header row] no\n");
     if (iConvertM2ToHectares == 1)
        printf("[convert m2 to hectares] yes\n\n");
     else
         printf("[convert m2 to hectares] no\n\n");

     // open input file
     readname = (char *) calloc(strlen(sInputFileName)+2, sizeof(char));
     strcpy(readname,sInputFileName);
     if ((infp = fopen(readname,"r"))==NULL)
     {
        printf("input matrix file %s not found\nAborting Program.",readname);
        exit(1);
     }
     free(readname);

     // create output file
     writename = (char *) calloc(strlen(sOutputFileName)+2, sizeof(char));
     strcpy(writename,sOutputFileName);
     if ((outfp = fopen(writename,"w"))==NULL)
     {
        printf("cannot create output matrix file %s\nAborting Program.",writename);
        exit(1);
     }
     free(writename);
     if (iSuppressHeaderRow == 0)
        fprintf(outfp,"species,pu,amount\n",sLine);
     // create output file species name
     writename = (char *) calloc(strlen(sOutputFileName)+strlen("spname_")+2, sizeof(char));
     strcpy(writename,"spname_");
     strcat(writename,sOutputFileName);
     if ((outspnamefp = fopen(writename,"w"))==NULL)
     {
        printf("cannot create output matrix file %s\nAborting Program.",writename);
        exit(1);
     }
     free(writename);
     if (iSuppressHeaderRow == 0)
        fprintf(outspnamefp,"speciesindex,speciesname\n");
     // create debug output file
     //writename = (char *) calloc(strlen("debug.txt")+2, sizeof(char));
     //strcpy(writename,"debug.txt");
     //if ((debugfp = fopen(writename,"w"))==NULL)
     //{
     //   printf("cannot create output debug file %s\nAborting Program.",writename);
     //   exit(1);
     //}
     //free(writename);

     // count how many species id's
     fgets(sLine,20000-1,infp);
     strtok(sLine," ,.;:^*\"/|\t\'\\\n");
     iSpeciesCount = 0;
     while ((sVarName = strtok(NULL," ,;:^*\"/|\t\'\\\n")) != NULL)
     {
           fprintf(outspnamefp,"%d,%s\n",iSpeciesCount+iBaseSpeciesIndex,sVarName);
           iSpeciesCount++;
     }

     // traverse input file, writing non-zero amounts to the output file
     while (fgets(sLine,20000-1,infp) != NULL)
     {
           InsertZeroesIntoString(sLine,sAdjustedLine);

           //printf("Line >%s<",sLine);
           //printf("Adjusted >%s<",sAdjustedLine);
           //scanf("%c");
           //fprintf(debugfp,">%s<\n",sAdjustedLine);

           // read the planning unit id from the line
           sVarName = strtok(sAdjustedLine," ,;:^*\"/|\t\'\\\n");
           sscanf(sVarName,"%d",&iPUID);

           // read the species amounts from the line
           i = 0;
           while ((sVarName = strtok(NULL," ,;:^*\"/|\t\'\\\n")) != NULL)
           {
                 sscanf(sVarName,"%lf",&rAmount);
                 if (rAmount > 0)
                 {
                    if (iConvertM2ToHectares == 1)
                       rReportAmount = rAmount / 10000;
                    else
                        rReportAmount = rAmount;

                    // see if amount is integer or floating point
		            //   output as appropriate type
		            rRoundAmount = round(rReportAmount);
		            if (rRoundAmount == rReportAmount)
		            {
		               iRoundAmount = lround(rReportAmount);
		               sprintf(sInputString,"%d,%d,%d",i+iBaseSpeciesIndex,iPUID,iRoundAmount);
		            }
		            else
		                sprintf(sInputString,"%d,%d,%lf",i+iBaseSpeciesIndex,iPUID,rReportAmount);
                    fprintf(outfp,"%s\n",sInputString);
                    iRecordCount++;
                 }
                 else
                     ;
                 i++;
           }
           iPUCount++;
     }

     // close files
     fclose(infp);
     fclose(outfp);
     fclose(outspnamefp);
     //fclose(debugfp);

     printf("conversion finished\n");
     printf("%d records processed\n",iRecordCount);
     printf("%d planning units\n",iPUCount);
     printf("%d species\n",iSpeciesCount);
}


void ExecuteMatrixConversion_Tabular_To_Relational(char sInputFileName[],char sOutputFileName[])
{
	 int i = 0, iSpeciesCount = 0, iSpeciesId, iScanfResult, iPUCount = 0, iRecordCount = 0;
	 FILE *infp,*outfp;
	 char *readname,*writename;
     char sLine[5000], sElement[500], sInputString[500],sOutputString[500];
     int *SPID, iPUID;
     char *sVarVal, *sVarName;
     double rAmount, rRoundAmount;
     long iRoundAmount;

     printf("%s\n\n",sVersionString);
	 printf("Conversion type is tabular format to relational format.\n");
	 printf("[input file name] %s\n",sInputFileName);
	 printf("[output file name] %s\n\n",sOutputFileName);

	 // open input file
     readname = (char *) calloc(strlen(sInputFileName)+2, sizeof(char));
	 strcpy(readname,sInputFileName);
	 if ((infp = fopen(readname,"r"))==NULL)
	 {
		printf("input matrix file %s not found\nAborting Program.",readname);
		exit(1);
     }
	 free(readname);

	 // count how many species id's
	 fgets(sLine,5000-1,infp);
	 strtok(sLine," ,.;:^*\"/|\t\'\\\n");
	 while ((sVarName = strtok(NULL," ,;:^*\"/|\t\'\\\n")) != NULL)
	       iSpeciesCount++;

	 // create array of species id's
     SPID = (int *) calloc(iSpeciesCount,sizeof(int));
	 // populate array of species id's
	 rewind(infp);
	 fgets(sLine,5000-1,infp);
	 strtok(sLine," ,.;:^*\"/|\t\'\\\n");
	 while ((sVarName = strtok(NULL," ,;:^*\"/|\t\'\\\n")) != NULL)
	 {
		   sscanf(sVarName,"%d",&SPID[i]);
	       i++;
 	 }

	 // create output file
     writename = (char *) calloc(strlen(sOutputFileName)+2, sizeof(char));
	 strcpy(writename,sOutputFileName);
	 if ((outfp = fopen(writename,"w"))==NULL)
	 {
		printf("cannot create output matrix file %s\nAborting Program.",writename);
		exit(1);
     }
	 free(writename);
     fprintf(outfp,"species,pu,amount\n",sLine);

     // traverse input file, writing non-zero amounts to the output file
     while (fgets(sLine,5000-1,infp) != NULL)
     {
		   // read the planning unit id from the line
		   sVarName = strtok(sLine," ,;:^*\"/|\t\'\\\n");
		   sscanf(sVarName,"%d",&iPUID);

		   // read the species amounts from the line
           i = 0;
           while ((sVarName = strtok(NULL," ,;:^*\"/|\t\'\\\n")) != NULL)
	       {
		         sscanf(sVarName,"%lf",&rAmount);
		         if (rAmount > 0)
		         {
					// see if amount is integer or floating point
		            //   output as appropriate type
		            rRoundAmount = round(rAmount);
		            if (rRoundAmount == rAmount)
		            {
		               iRoundAmount = lround(rAmount);
		               sprintf(sInputString,"%d,%d,%d",SPID[i],iPUID,iRoundAmount);
		            }
		            else
		                sprintf(sInputString,"%d,%d,%lf",SPID[i],iPUID,rAmount);
					fprintf(outfp,"%s\n",sInputString);
		            iRecordCount++;
			     }
	             i++;
 	       }
 	       iPUCount++;
	 }

     // dispose array
     free(SPID);
     // close files
     fclose(infp);
     fclose(outfp);

	 printf("conversion finished\n");
	 printf("%d records processed\n",iRecordCount);
	 printf("%d planning units\n",iPUCount);
	 printf("%d species\n",iSpeciesCount);
}

void siftDown_SPorder(struct matrixstruct numbers[], int root, int bottom, int array_size)
{
     int done, maxChild;
     typematrixstruct temp;

     done = 0;
     while ((root*2 <= bottom) && (!done))
     {
	       if (root*2 < array_size)
	       {
              if (root*2 == bottom)
                 maxChild = root * 2;
              else if (numbers[root * 2].spno > numbers[root * 2 + 1].spno)
                      maxChild = root * 2;
              else
                  maxChild = root * 2 + 1;

              if (numbers[root].spno < numbers[maxChild].spno)
              {
                 temp = numbers[root];
                 numbers[root] = numbers[maxChild];
                 numbers[maxChild] = temp;
                 root = maxChild;
              }
              else
                  done = 1;
	       }
	       else
	           done = 1;
     }
}

void heapSort_SPorder(struct matrixstruct numbers[], int array_size)
{
  int i;
  typematrixstruct temp;

  for (i = (array_size / 2)-1; i >= 0; i--)
    siftDown_SPorder(numbers, i, array_size-1,array_size);

  for (i = array_size-1; i >= 1; i--)
  {
    temp = numbers[0];
    numbers[0] = numbers[i];
    numbers[i] = temp;
    siftDown_SPorder(numbers, 0, i-1,array_size);
  }
}

void siftDown_PUorder(struct matrixstruct numbers[], int root, int bottom, int array_size)
{
     int done, maxChild;
     typematrixstruct temp;

     //if (iDebugSort == 1)
     //{
     //   fprintf(debugfp,"siftDown_PUorder start %d %d\n",root,bottom);
     //}

     done = 0;
     while ((root*2 <= bottom) && (!done))
     {
		   if (root*2 < array_size)
	       {
              if (root*2 == bottom)
                 maxChild = root * 2;
              else if (numbers[root * 2].puno > numbers[root * 2 + 1].puno)
                      maxChild = root * 2;
                   else
                       maxChild = root * 2 + 1;

              if (numbers[root].puno < numbers[maxChild].puno)
              {
                 temp = numbers[root];
                 numbers[root] = numbers[maxChild];
                 numbers[maxChild] = temp;

                 //if (iDebugSort == 1)
                 //{
                 //   sprintf(sDebugLine,"siftDown_PUorder swap %d : %lf,%d,%d  %d : %lf,%d,%d\n"
                 //                     ,root,numbers[root].amount,numbers[root].puno,numbers[root].spno
                 //                     ,maxChild,numbers[maxChild].amount,numbers[maxChild].puno,numbers[maxChild].spno);
                 //   fprintf(debugfp,sDebugLine);
                 //}

                 root = maxChild;
              }
              else
                  done = 1;
           }
           else
               done = 1;
     }

     //if (iDebugSort == 1)
     //{
     //   fprintf(debugfp,"siftDown_PUorder end\n");
     //}
}

void heapSort_PUorder(struct matrixstruct numbers[], int array_size)
{
  int i;
  typematrixstruct temp;

  if (iDebugSort == 1)
  {
     fprintf(debugfp,"heapSort_PUorder start, array size is %d\n",array_size);
  }

  for (i = (array_size / 2)-1; i >= 0; i--)
    siftDown_PUorder(numbers, i, array_size-1,array_size);

  for (i = array_size-1; i >= 1; i--)
  {
    temp = numbers[0];
    numbers[0] = numbers[i];
    numbers[i] = temp;

    if (iDebugSort == 1)
    {
	   sprintf(sDebugLine,"heapSort_PUorder swap %d : %lf,%d,%d  %d : %lf,%d,%d\n"
                      ,0,numbers[0].amount,numbers[0].puno,numbers[0].spno
                      ,i,numbers[i].amount,numbers[i].puno,numbers[i].spno);
       fprintf(debugfp,sDebugLine);
    }

    siftDown_PUorder(numbers, 0, i-1,array_size);
  }

  if (iDebugSort == 1)
  {
     fprintf(debugfp,"heapSort_PUorder end\n");
  }
}

void ExecuteMatrixConversion_order(char sInputFileName[],char sOutputFileName[],int iOrderType)
{
     // conversion types 2, 3, 4, 6, and 7

	 int iLines = 0, i = 0;
	 FILE *infp,*outfp;
	 char *readname,*writename;
     char sLine[500];
     typematrixstruct *SM;
     char *sVarVal, sInputString[500],sOutputString[500], sAmount[100], sProb[100];
     double rRoundAmount;

     printf("%s\n\n",sVersionString);
     if (iOrderType == 2)
	    printf("Conversion type is species order to planning unit order.\n");
	 else
     if (iOrderType == 3)
	     printf("Conversion type is planning unit order to species order.\n");
	 else
     if (iOrderType == 4)
        printf("Conversion type is species order to planning unit order, input field order is pu,species,amount.\n");
     else
     if (iOrderType == 6)
	    printf("Conversion type is MarProb species order to planning unit order.\n");     
     else
     if (iOrderType == 7)
	     printf("Conversion type is MarProb planning unit order to species order.\n");

	 printf("[input file name] %s\n",sInputFileName);
	 printf("[output file name] %s\n\n",sOutputFileName);

	 // open input file
     readname = (char *) calloc(strlen(sInputFileName)+2, sizeof(char));
	 strcpy(readname,sInputFileName);
	 if ((infp = fopen(readname,"r"))==NULL)
	 {
		printf("input matrix file %s not found\nAborting Program.",readname);
		exit(1);
     }
	 free(readname);

	 // count lines in input file
	 fgets(sLine,500-1,infp);
	 while (fgets(sLine,500-1,infp)){
	       iLines++;
	 }
	 // rewind input file
	 rewind(infp);
	 fgets(sLine,500-1,infp);
	 // create output file
     writename = (char *) calloc(strlen(sOutputFileName)+2, sizeof(char));
	 strcpy(writename,sOutputFileName);
	 if ((outfp = fopen(writename,"w"))==NULL)
	 {
		printf("cannot create output matrix file %s\nAborting Program.",writename);
		exit(1);
     }
	 free(writename);
	 if (iOrderType > 4)
	    fprintf(outfp,"species,pu,amount,prob\n");
	 else
         fprintf(outfp,"species,pu,amount\n");

     // create array
	 SM = (typematrixstruct *) calloc(iLines,sizeof(typematrixstruct));

     // read input file to array
	 while (fgets(sLine,500-1,infp)){

		   sVarVal = strtok(sLine," ,;:^*\"/|\t\'\\\n");
           if (iOrderType == 4)
              sscanf(sVarVal,"%d",&SM[i].puno);
           else
		       sscanf(sVarVal,"%d",&SM[i].spno);
		   sVarVal = strtok(NULL," ,;:^*\"/|\t\'\\\n");
           if (iOrderType == 4)
              sscanf(sVarVal,"%d",&SM[i].spno);
           else
    		   sscanf(sVarVal,"%d",&SM[i].puno);
		   sVarVal = strtok(NULL," ,;:^*\"/|\t\'\\\n");
		   sscanf(sVarVal,"%lf",&SM[i].amount);
		   if (iOrderType > 4)
		   {
		      sVarVal = strtok(NULL," ,;:^*\"/|\t\'\\\n");
		      sscanf(sVarVal,"%lf",&SM[i].prob);
		   }

	       i++;
	 }

     // sort array
     if (iOrderType == 2)
        // SPorder_To_PUorder
        heapSort_PUorder(SM,iLines);
     else
     if (iOrderType == 4)
        // SPorder_To_PUorder
        heapSort_PUorder(SM,iLines);
     else
     if (iOrderType == 6)
        // SPorder_To_PUorder
        heapSort_PUorder(SM,iLines);
     else
     if (iOrderType == 7)
        // PUorder_To_SPorder
        heapSort_SPorder(SM,iLines);
     else
         // PUorder_To_SPorder
         heapSort_SPorder(SM,iLines);

     // write array to output file
     for (i=0;i<iLines;i++)
     {
		 // see if amount is integer or floating point
		 //   output as appropriate type
		 rRoundAmount = round(SM[i].amount);
		 
		 if (rRoundAmount == SM[i].amount)
		    sprintf(sAmount,"%d",lround(SM[i].amount));
		 else
		     sprintf(sAmount,"%lf",SM[i].amount);
         
         if (iOrderType > 4)
         {
            rRoundAmount = round(SM[i].prob);
            if (rRoundAmount == SM[i].prob)
               sprintf(sProb,"%d",lround(SM[i].prob));
            else
                sprintf(sProb,"%lf",SM[i].prob);
                
            sprintf(sInputString,"%d,%d,%s,%s",SM[i].spno,SM[i].puno,sAmount,sProb);
         }
         else
             sprintf(sInputString,"%d,%d,%s",SM[i].spno,SM[i].puno,sAmount);
         
		 fprintf(outfp,"%s\n",sInputString);
     }

     // dispose array
     free(SM);
     // close files
	 fclose(infp);
	 fclose(outfp);

	 printf("conversion finished\n");
	 printf("%d records processed\n",iLines);
}

void display_usage();

void display_usage()
{
     // no arguments specified or incorrect number of arguments
     fprintf(stderr,"\n");
     fprintf(stderr,"NOTE: Input field order is assumed to be species,pu,amount\n\n");
     fprintf(stderr,"Usage: convert_mtx [conversion type] [input file name] [output file name]\n");
     fprintf(stderr,"                   {value} {suppress header row} {convert m2 to hectares}\n\n");
     fprintf(stderr,"Where conversion type is:\n\n");
     fprintf(stderr,"  1 Tabular format to relational format.\n");
     fprintf(stderr,"  2 Species order to planning unit order.\n");
     fprintf(stderr,"  3 Planning unit to species order order.\n");
     fprintf(stderr,"  4 Species order to planning unit order,\n    input field order is pu,species,amount.\n");
     fprintf(stderr,"  5 Tabular format to relational format,\n    add value to base species index.\n");
     fprintf(stderr,"  6 MarProb Species order to planning unit order.\n");
     fprintf(stderr,"  7 MarProb Planning unit to species order order.\n");     
     fprintf(stderr,"\nParameters in {curly brackets} apply only to conversion type 5.\n");

     exit(1);
}

int main(int argc,char *argv[])
{
	char sInputFileName[100], sOutputFileName[100], sConversionType[100], sBaseSpeciesIndex[100], sSuppressHeaderRow[100], *endptr;
	long iConversionType, iBaseSpeciesIndex, iSuppressHeaderRow, iConvertM2ToHectares;
	char *writename;

    printf("\n        %s \n\n",sVersionString);
    printf("   Written and coded by Matt Watts\n\n");
    printf("   %s\n\n",sMattWattsEmail);

	if (argc < 4)
       display_usage();
	else
	{
		// handle the program options
	    strcpy(sConversionType,argv[1]);
		iConversionType = strtol(sConversionType,&endptr,10);
		strcpy(sInputFileName,argv[2]);
		strcpy(sOutputFileName,argv[3]);
    }

    if (iDebugSort == 1)
	{
	   // create output debug file
	   writename = (char *) calloc(strlen("debug_log.txt")+2, sizeof(char));
	   strcpy(writename,"debug_log.txt");
	   if ((debugfp = fopen(writename,"w"))==NULL)
	   {
	      printf("cannot create output matrix file %s\nAborting Program.",writename);
	      exit(1);
	   }
	   free(writename);
	   fprintf(debugfp,"main start\n\n");
	}

    if ((iConversionType > 7) || (iConversionType < 1))
       display_usage();
    else
	if (iConversionType == 1)
	   ExecuteMatrixConversion_Tabular_To_Relational(sInputFileName,sOutputFileName);
	else
    if (iConversionType == 5)
    {
       strcpy(sBaseSpeciesIndex,argv[4]);
       iBaseSpeciesIndex = strtol(sBaseSpeciesIndex,&endptr,10);
       if (argc > 5)
          iSuppressHeaderRow = 1;
       else
           iSuppressHeaderRow = 0;
       if (argc > 6)
          iConvertM2ToHectares = 1;
       else
           iConvertM2ToHectares = 0;
       ExecuteMatrixConversion_Tabular_To_Relational_AddSpeciesIndex(sInputFileName,sOutputFileName,iBaseSpeciesIndex,iSuppressHeaderRow,iConvertM2ToHectares);
    }
    else
	    ExecuteMatrixConversion_order(sInputFileName,sOutputFileName,iConversionType);

    if (iDebugSort == 1)
	{
	   fprintf(debugfp,"\nmain end\n");
	   fclose(debugfp);
	}

	return 0;
}

