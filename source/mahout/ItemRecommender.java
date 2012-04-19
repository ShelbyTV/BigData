import java.io.File;
import java.io.IOException;
import java.util.List;

import org.apache.mahout.cf.taste.common.TasteException;
import org.apache.mahout.cf.taste.impl.model.file.FileDataModel;
import org.apache.mahout.cf.taste.impl.recommender.GenericBooleanPrefItemBasedRecommender;
import org.apache.mahout.cf.taste.impl.similarity.LogLikelihoodSimilarity;
import org.apache.mahout.cf.taste.recommender.ItemBasedRecommender;
import org.apache.mahout.cf.taste.recommender.RecommendedItem;
import org.apache.mahout.cf.taste.similarity.ItemSimilarity;
import org.apache.mahout.cf.taste.impl.common.LongPrimitiveIterator;

public class ItemRecommender 
{
      public static void main(String args[])
      {
            try
            {
                  // Data model created to accept the input file
                  FileDataModel dataModel = new FileDataModel(new File("../mongoexport/broadcasts_pruned2.csv"));
                 
                  /*Specifies the Similarity algorithm*/
                  ItemSimilarity itemSimilarity = new LogLikelihoodSimilarity(dataModel);
                 
                  /*Initalizing the recommender */
                  ItemBasedRecommender recommender = new GenericBooleanPrefItemBasedRecommender(dataModel, itemSimilarity);

		  LongPrimitiveIterator iter = dataModel.getItemIDs();
                  
                  int max = 151369;
                  int count = 0;

                  while (iter.hasNext() && count < max)
                  {                
			  long item = iter.nextLong();
                          count++;
 
			  //calling the recommend method to generate recommendations
			  List<RecommendedItem> recommendations = recommender.mostSimilarItems(item, 20);
           
			  for (RecommendedItem recommendedItem : recommendations)
				  System.out.println(String.format("%d %d %f", item, recommendedItem.getItemID(), recommendedItem.getValue()));
		  }
            }
            catch (IOException e) {
                  // TODO Auto-generated catch block
                  e.printStackTrace();
            } catch (TasteException e) {
                  // TODO Auto-generated catch block
                  e.printStackTrace();
            }
      }
}
